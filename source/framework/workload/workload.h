/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/common_help_message.h"
#include "framework/workload/workload.h"
#include "framework/workload/workload_argument_container.h"
#include "framework/workload/workload_io.h"
#include "framework/workload/workload_statistics.h"
#include "framework/workload/workload_synchronization.h"

#include <functional>

template <typename _ArgumentContainerT>
class Workload {
  public:
    using ArgumentContainerT = _ArgumentContainerT;
    static_assert(std::is_base_of_v<WorkloadArgumentContainer, ArgumentContainerT>, "Parameters class should derive from WorkloadArgumentContainer");
    using WorkloadImplementation = std::function<TestResult(const ArgumentContainerT &, Statistics &, WorkloadSynchronization &, WorkloadIo &)>;
    using ProcessResult = int;

    static inline WorkloadImplementation implementation = {};

    ProcessResult runFromCommandLine(int argc, char **argv) {
        Configuration::loadDefaultConfiguration();

        CommandLineArguments commandLineArguments = {};
        std::string commandLineArgumentsParsingErrors = {};
        if (!CommandLineArgument::parseArguments(argc, argv, commandLineArguments, commandLineArgumentsParsingErrors)) {
            std::cerr << commandLineArgumentsParsingErrors << std::endl;
            return 1;
        }

        ArgumentContainerT arguments{};

        if (!arguments.parseArguments(commandLineArguments)) {
            std::cerr << "Error parsing command line" << std::endl;
            return 1;
        }

        bool error = false;

        if (const auto unparsedArgs = arguments.getUnparsedArguments(); !unparsedArgs.empty()) {
            const auto getKey = +[](const Argument *a) { return a->getKey(); };
            std::cerr << CommonHelpMessage::errorUnsetArguments() << joinStrings(", ", unparsedArgs, getKey) << std::endl;
            error = true;
        }

        if (const auto unprocessedArgs = CommandLineArgument::getUnprocessedArguments(commandLineArguments); !unprocessedArgs.empty()) {
            const auto getKey = +[](const CommandLineArgument *a) { return a->getKey(); };
            std::cerr << CommonHelpMessage::errorIgnoredCommandLineArgs() << joinStrings(", ", unprocessedArgs, getKey) << std::endl;
            error = true;
        }

        if (error) {
            return toProcessResult(TestResult::InvalidArgs);
        }

        return run(arguments);
    }

    ProcessResult run(const ArgumentContainerT &arguments) {
        WorkloadStatistics statistics{arguments.iterations};
        WorkloadSynchronization synchronization{arguments.iterations, arguments.synchronize};
        std::unique_ptr<WorkloadIo> io = WorkloadIo::create(arguments);
        TestResult result = runImpl(arguments, statistics, synchronization, *io);
        if (result == TestResult::Success) {
            DEVELOPER_WARNING_IF(!statistics.isFull(), "test did not generate as many values as expected");
            DEVELOPER_WARNING_IF(!synchronization.validate(), "test did not synchronize the correct amount of times");
            statistics.printStatistics(*io);
        } else {
            synchronization.executeRemainingSynchronizations(*io);
        }
        return toProcessResult(result);
    }

  private:
    TestResult runImpl(const ArgumentContainerT &arguments, WorkloadStatistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {
        if (implementation == nullptr) {
            return TestResult::NoImplementation;
        }

        if (!arguments.validateArguments()) {
            return TestResult::InvalidArgs;
        }

        return implementation(arguments, statistics, synchronization, io);
    }

    static ProcessResult toProcessResult(TestResult testResult) {
        return static_cast<ProcessResult>(testResult);
    }
};
