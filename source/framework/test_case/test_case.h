/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/benchmark_info.h"
#include "framework/supported_apis.h"
#include "framework/test_case/test_case_argument_container.h"
#include "framework/test_case/test_case_base.h"
#include "framework/test_case/test_case_statistics.h"
#include "framework/test_case/test_result.h"
#include "framework/test_map.h"
#include "framework/utility/common_help_message.h"
#include "framework/utility/error.h"
#include "framework/utility/string_utils.h"

#include <functional>
#include <iostream>
#include <sstream>
#include <type_traits>

// This is a base class for every test case written for any compute benchmark. Its template
// parameter represents a concrete class containing actual arguments from a given test case.
// It must be derived from the TestCaseArgumentContainer class.
//
// All methods, which do not require access to the concrete argument class and can work with
// just TestCaseArgumentContainer, should be implemented in TestCaseBase. That's because this
// file is included in majority .cpp files in the project and must be recompiled many times.
template <typename _ArgumentContainer>
class TestCase : public TestCaseBase {
  public:
    using ArgumentContainerT = _ArgumentContainer;
    static_assert(std::is_base_of_v<TestCaseArgumentContainer, ArgumentContainerT>, "Arguments class should derive from TestCaseArgumentContainer");

    struct BenchmarkImplementation {
        using Function = std::function<TestResult(ArgumentContainerT, Statistics &)>;
        Function function = {};
        bool requiresIntelExtensions = false;
    };
    static inline BenchmarkImplementation implementations[(int)Api::COUNT];

    std::unique_ptr<ArgumentContainer> getArguments() const override { return std::make_unique<ArgumentContainerT>(); }
    std::string getHelpParameters() const override { return ArgumentContainerT{}.getHelp(2u); }

    bool isApiImplemented(Api api) const override {
        return implementations[static_cast<int>(api)].function != nullptr;
    }

    bool runFromCommandLine(CommandLineArguments &commandLineArguments) override {
        // Parse test-specific parameters
        ArgumentContainerT arguments;
        bool error = false;
        if (!parseArguments(arguments, commandLineArguments)) {
            return false;
        }

        // Check if all command line arguments were processed (no ignoring)
        if (const auto unprocessedArgs = CommandLineArgument::getUnprocessedArguments(commandLineArguments); !unprocessedArgs.empty()) {
            const auto getKey = +[](const CommandLineArgument *a) { return a->getKey(); };
            std::cerr << CommonHelpMessage::errorIgnoredCommandLineArgs() << joinStrings(", ", unprocessedArgs, getKey) << std::endl;
            error = true;
        }

        // Check if all test case arguments were set (no defaults)
        if (const auto unparsedArgs = arguments.getUnparsedArguments(); !unparsedArgs.empty()) {
            const auto getKey = +[](const Argument *a) { return a->getKey(); };
            std::cerr << CommonHelpMessage::errorUnsetArguments() << joinStrings(", ", unparsedArgs, getKey) << std::endl;
            error = true;
        }

        if (error) {
            return false;
        }

        // Try running with all possible APIs. If some are disabled, e.g. --api=ocl is passed, then the rest will be skipped in run() method
        if (!Configuration::get().noColumnNames) {
            TestCaseStatistics::printStatisticsHeader(Configuration::get().printType);
        }
        for (int apiIndex = static_cast<int>(Api::FIRST); apiIndex <= static_cast<int>(Api::LAST); apiIndex++) {
            arguments.api = static_cast<Api>(apiIndex);
            run(arguments);
        }
        return true;
    }

    void run(ArgumentContainerT &arguments) const {
        arguments.iterations = Configuration::get().iterations;
        arguments.noIntelExtensions = Configuration::get().noIntelExtensions;

        // Create statistics object
        const auto testCaseNameWithConfig = getTestCaseNameWithConfig(arguments, Configuration::get().dumpCommandLines);
        TestCaseStatistics statistics{arguments.iterations, Configuration::get().printType};

        // Run test
        const auto testResult = runImpl(statistics, arguments, testCaseNameWithConfig);
        if (testResult == TestResult::Success) {
            DEVELOPER_WARNING_IF(!statistics.isFull(), "test did not generate as many values as expected");
            statistics.printStatistics(testCaseNameWithConfig);
            if (Configuration::get().sleepFor > 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(Configuration::get().sleepFor));
            }
        } else if (testResult == TestResult::Nooped) {
            statistics.printStatistics(testCaseNameWithConfig);
        } else {
            const auto &testResultInfo = TestResultHelper::getTestResultInfo(testResult);

            // If test was skipped at the very beginning, it shouldn't have pushed any statistics
            DEVELOPER_WARNING_IF(testResultInfo.wasTestSkipped && !statistics.isEmpty(), "test was skipped but generated some values");

            // Print output line with error info if needed
            const auto printMessage = (Configuration::get().printAllResults && testResultInfo.printInPrintAllResultsMode) ||
                                      (arguments.isSingleTestMode ? testResultInfo.printInSingleTestMode : testResultInfo.printInAllTestsMode);
            if (printMessage) {
                statistics.printStatisticsString(testCaseNameWithConfig, testResultInfo.stringMessage);
            }
        }
    }

  private:
    TestResult runImpl(TestCaseStatistics &statistics, const ArgumentContainerT &arguments, const std::string &testCaseNameWithConfig) const {
        // Check test filters and arg filters
        if (!matchesWithTestFilter()) {
            return TestResult::FilteredOut;
        }

        if (!matchesWithArgFilter(arguments)) {
            return TestResult::FilteredOut;
        }

        if (needsToBeFilteredDueToLimitedTargets()) {
            return TestResult::FilteredOut;
        }

        // Get API
        const auto selectedApi = Configuration::get().selectedApi;
        if (arguments.api != selectedApi && selectedApi != Api::All) {
            return TestResult::SkippedApi;
        }
        if (!SupportedApis::isApiSupported(arguments.api)) {
            return TestResult::UnsupportedApi;
        }

        // Get implementation
        const auto apiIndex = static_cast<int>(arguments.api);
        const auto &benchmarkImplementation = implementations[apiIndex];
        if (benchmarkImplementation.function == nullptr) {
            return TestResult::NoImplementation;
        }

        // Check if test needs Intel extensions
        if (arguments.noIntelExtensions && benchmarkImplementation.requiresIntelExtensions) {
            return TestResult::IntelExtensionsRequired;
        }

        // Validate arguments
        if (!arguments.validateArguments()) {
            return TestResult::InvalidArgs;
        }

        // Verify if current test case is added to the test map
        const auto &testMap = TestMap::get();
        if (testMap.find(getTestCaseName()) == testMap.end()) {
            printTestMapWarning();
        }

        // Check if test case name with config is not too long
        if (!Configuration::get().dumpCommandLines && !arguments.isSingleTestMode) {
            printTestCaseNameLengthWarning(testCaseNameWithConfig);
        }

        // Run the test
        if (Configuration::get().interactivePrints) {
            // This will print test name before running the actual test along with '\r' character,
            // so it will be overwritten in next step.
            statistics.printStatisticsBeforeTest(testCaseNameWithConfig);
        }
        const TestResult testResult = benchmarkImplementation.function(arguments, statistics);
        if (Configuration::get().interactivePrints) {
            // This will overwrite the test name, because it was only a temporal caption.
            statistics.printClearLineAfterTest();
        }
        return testResult;
    }
};
