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

#include "../utility/mad_performance.h"

struct DoublePrecisionPerformanceArguments : TestCaseArgumentContainer {
    BooleanArgument useEvents;
    VectorWidthArgument vectorWidth;

    DoublePrecisionPerformanceArguments()
        : useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          vectorWidth(*this, "vectorWidth", MadPerformance::getVectorWidthHelp()) {}
};

struct DoublePrecisionPerformance : TestCase<DoublePrecisionPerformanceArguments> {
    using TestCase<DoublePrecisionPerformanceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DoublePrecisionPerformance";
    }

    std::string getHelp() const override {
        return "measures double precision floating point throughput in GFLOPS by executing chains of mad operations on double or doubleN vectors in a kernel, dispatch size is determined automatically from device capabilities";
    }
};
