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

struct SinglePrecisionVectorPerformanceArguments : TestCaseArgumentContainer {
    BooleanArgument useEvents;
    MultiLaneVectorWidthArgument vectorWidth;

    SinglePrecisionVectorPerformanceArguments()
        : useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          vectorWidth(*this, "vectorWidth", MadPerformance::getMultiLaneVectorWidthHelp()) {}
};

struct SinglePrecisionVectorPerformance : TestCase<SinglePrecisionVectorPerformanceArguments> {
    using TestCase<SinglePrecisionVectorPerformanceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SinglePrecisionVectorPerformance";
    }

    std::string getHelp() const override {
        return "measures single precision floating point throughput in GFLOPS by executing chains of mad operations on floatN vectors in a kernel, dispatch size is determined automatically from device capabilities";
    }
};
