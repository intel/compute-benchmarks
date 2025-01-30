/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "benchmark_main.h"

#include "framework/benchmark_info.h"
#include "framework/configuration.h"
#include "framework/gtest_event_listener.h"
#include "framework/print_device_info.h"
#include "framework/test_map.h"
#include "framework/utility/common_help_message.h"
#include "framework/utility/string_utils.h"
#include "framework/utility/working_directory_helper.h"

#include <gtest/gtest.h>
#include <iostream>

int BenchmarkMain::printVersion(bool enableWarning, const char *prefix) {
    if (!benchmarkVersion.empty()) {
        std::cout << prefix << benchmarkVersion << std::endl;
        return 0;
    }

    if (enableWarning) {
        std::cerr << "Unknown version. Run CMake with \"-D INCLUDE_VERSION=ON\" to include it in the binary." << std::endl;
        return 1;
    }

    return 0;
}

int BenchmarkMain::generateDocs() {
    const std::string illegalCharacters = " -:='\"<>|{}[]/.,?\\+$";

    // Print benchmark name and description
    const std::string benchmarkName = BenchmarkInfo::get().getBenchmarkName();
    if (containsIllegalCharacters(benchmarkName, illegalCharacters)) {
        std::cerr << "ERROR: benchmark name contains invalid characters.";
        return 1;
    }
    const std::string benchmarkDescription = BenchmarkInfo::get().getBenchmarkDescription();
    if (containsIllegalCharacters(benchmarkDescription, ";")) {
        std::cerr << "ERROR: benchmark description contains invalid characters.";
        return 1;
    }
    std::cout << benchmarkName << ";" << benchmarkDescription << '\n';

    // Print test cases
    for (const auto &testEntry : TestMap::get()) {
        const std::unique_ptr<TestCaseInterface> &testCase = testEntry.second;
        if (testCase->getApisWithImplementation().empty()) {
            continue;
        }

        // Print test case name and help message.
        const std::string testCaseName = testCase->getTestCaseName();
        if (containsIllegalCharacters(testCaseName, illegalCharacters)) {
            std::cerr << "ERROR: test case \"" << testCaseName << "\" contains illegal characters.";
            return 1;
        }
        const std::string testCaseHelp = testCase->getHelp();
        if (containsIllegalCharacters(testCaseHelp, ";")) {
            std::cerr << "ERROR: help message of test case \"" << testCaseName << "\" contains illegal characters.";
            return 1;
        }
        std::cout << " " << testCaseName << ";" << testCaseHelp << '\n';

        // Print test case arguments
        const std::unique_ptr<ArgumentContainer> arguments = testCase->getArguments();
        for (const Argument *argument : arguments->getArguments()) {
            const std::string argumentKey = argument->getKey();
            if (containsIllegalCharacters(argumentKey, illegalCharacters)) {
                std::cerr << "ERROR: argument \"" << argumentKey << "\" in test case\"" << testCaseName << "\" contains illegal characters.";
                return 1;
            }
            const std::string argumentHelp = argument->getExtraHelp();
            if (containsIllegalCharacters(argumentKey, ";")) {
                std::cerr << "ERROR: help message of argument \"" << argumentKey << "\" in test case\"" << testCaseName << "\" contains illegal characters.";
                return 1;
            }
            std::cout << "  " << argumentKey << ";" << argumentHelp << '\n';
        }
    }

    return 0;
}

BenchmarkMain::BenchmarkMain(int argc, char **argv, const std::string benchmarkVersion)
    : argc(argc),
      argv(argv),
      benchmarkVersion(benchmarkVersion) {}

int BenchmarkMain::executeSingleTest(const std::string &testName) {
    const auto &testMap = TestMap::get();
    auto it = testMap.find(testName);
    if (it == testMap.end()) {
        std::cerr << "Unknown test case\n";
        return 1;
    }

    TestCaseInterface *testCase = it->second.get();
    replaceGtestListener<SingleTestGtestListener>();
    if (!testCase->runFromCommandLine(commandLineArguments)) {
        std::cerr << "Error parsing command line\n";
        return 1;
    }
    return 0;
}

int BenchmarkMain::executeAllTests() {
    for (auto &commandLineArgument : commandLineArguments) {
        if (commandLineArgument.getKey().find("gtest_") == 0) {
            commandLineArgument.markAsProcessed();
        }
    }

    if (const auto unprocessedArgs = CommandLineArgument::getUnprocessedArguments(commandLineArguments); !unprocessedArgs.empty()) {
        const auto getKey = +[](const CommandLineArgument *a) { return a->getKey(); };
        std::cerr << CommonHelpMessage::errorIgnoredCommandLineArgs() << joinStrings(", ", unprocessedArgs, getKey) << std::endl;
        return 1;
    }

    replaceGtestListener<AllTestsGtestListener>();
    return RUN_ALL_TESTS();
}

void BenchmarkMain::printHelp() {
    const auto filename = BenchmarkInfo::get().getBenchmarkFilename();
    // clang-format off
    std::cout << BenchmarkInfo::get().getBenchmarkDescription() << "\n"
                 "\n"
                 "The benchmark works in two modes - all-tests mode and single-test mode. They are further described below. "
                 "Global parameters applicable for both modes:\n"
                 << Configuration::get().getHelp(1u) << "\n"
                 "\n"
                 "First mode is the default and it runs all available benchmarks in many predefined configurations. Underlying test engine "
                 "is googletest, so standard googletest arguments like --gtest_filter can be used, if necessary.\n"
                 "\n"
                 "Second mode runs one specific benchmark with custom parameter values. Running benchmarks in this fashion requires "
                 "using --test argument, along with benchmark-specific parameters. All parameters have to be specified, there are no "
                 "default values.\n"
                 "\n"
                 "Example invocations:\n"
                 "\t" << filename << "                                                runs all possible tests\n"
                 "\t" << filename << " --api=ocl                                      runs all possible OpenCL tests\n"
                 "\t" << filename << " --iterations=100 --csv                         runs all possible tests with 100 iterations and dumps results as CSV\n"
                 "\t" << filename << " --gtest_filter=<regex>                         runs all tests matching a regular expression\n"
                 "\t" << filename << " --gtest_filter=*TestName*                      runs a test named \"TestName\" in all predefined configurations\n"
                 "\t" << filename << " --test=TestName --someParam=1 --otherParam=30  runs a test named \"TestName\" with specified parameters\n"
                 "\n"
                "All available test cases with their parameters:\n";
    // clang-format on
    for (const auto &entry : TestMap::get()) {
        const TestCaseInterface &testCase = *entry.second.get();
        const std::vector<Api> apis = testCase.getApisWithImplementation();
        if (apis.size() == 0) {
            continue;
        }

        std::cout << '\t' << testCase.getTestCaseName() << " - " << testCase.getHelp();
        std::cout << " Supported compute APIs: " << joinStrings(", ", apis, getUserFriendlyApiName) << ".";

        const std::string helpParameters = testCase.getHelpParameters();
        if (helpParameters.size() != 0) {
            std::cout << " Parameters:\n"
                      << helpParameters;
        } else {
            std::cout << '\n';
        }
        std::cout << '\n';
    }
}

int BenchmarkMain::setupEnvironment() {
    // Kernels will be loaded from the CWD, so we need to ensure we're in the right directory.
    WorkingDirectoryHelper::changeDirectoryToExeDirectory();

    // Each command line argument must be parsed and validated.
    std::string commandLineArgumentsParsingErrors = {};
    if (!CommandLineArgument::parseArguments(argc, argv, commandLineArguments, commandLineArgumentsParsingErrors)) {
        std::cerr << commandLineArgumentsParsingErrors << std::endl;
        return 1;
    }

    // Based on parsed command line arguments we can initialize Configuration, which contains global toggles
    // useful for various debug activities.
    if (!Configuration::parseArgumentsForConfiguration(commandLineArguments)) {
        std::cerr << "Error parsing command line\n";
        return 1;
    }

    return 0;
}

int BenchmarkMain::main() {
    // Perform general setup
    if (const int result = setupEnvironment(); result != 0) {
        return result;
    }

    // Run diagnostic activities
    const Configuration &configuration = Configuration::get();
    if (configuration.generateDocs) {
        return generateDocs();
    }
    if (configuration.hwInfo) {
        DeviceInfo::printAvailableDevices();
        return 0;
    }
    if (configuration.help) {
        printHelp();
        return 0;
    }
    if (configuration.version) {
        return printVersion(true);
    }

    // Run tests
    if (!Configuration::get().noHeaders) {
        DeviceInfo::printDeviceInfo();
        printVersion(false, "Benchmark version: ");
    }
    ::testing::InitGoogleTest(&argc, argv);
    if (std::string test = configuration.test; test != "") {
        return executeSingleTest(test);
    } else {
        return executeAllTests();
    }
}
