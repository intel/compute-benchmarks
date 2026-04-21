/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct SinglePrecisionPerformanceArguments : TestCaseArgumentContainer {
    BooleanArgument useEvents;

    SinglePrecisionPerformanceArguments()
        : useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct SinglePrecisionPerformance : TestCase<SinglePrecisionPerformanceArguments> {
    using TestCase<SinglePrecisionPerformanceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SinglePrecisionPerformance";
    }

    std::string getHelp() const override {
        return "measures single precision floating point throughput in GFLOPS by executing FMA operations in a kernel, dispatch size is determined automatically from device capabilities";
    }
};
