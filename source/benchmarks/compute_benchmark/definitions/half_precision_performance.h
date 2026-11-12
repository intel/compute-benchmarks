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

struct HalfPrecisionPerformanceArguments : TestCaseArgumentContainer {
    BooleanArgument useEvents;
    VectorWidthArgument vectorWidth;

    HalfPrecisionPerformanceArguments()
        : useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          vectorWidth(*this, "vectorWidth", MadPerformance::getVectorWidthHelp()) {}
};

struct HalfPrecisionPerformance : TestCase<HalfPrecisionPerformanceArguments> {
    using TestCase<HalfPrecisionPerformanceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "HalfPrecisionPerformance";
    }

    std::string getHelp() const override {
        return "measures half precision floating point throughput in GFLOPS by executing chains of mad operations on half or halfN vectors in a kernel, dispatch size is determined automatically from device capabilities";
    }
};
