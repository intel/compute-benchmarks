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

struct IntegerPerformanceArguments : TestCaseArgumentContainer {
    BooleanArgument useEvents;
    VectorWidthArgument vectorWidth;

    IntegerPerformanceArguments()
        : useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          vectorWidth(*this, "vectorWidth", MadPerformance::getVectorWidthHelp()) {}
};

struct IntegerPerformance : TestCase<IntegerPerformanceArguments> {
    using TestCase<IntegerPerformanceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "IntegerPerformance";
    }

    std::string getHelp() const override {
        return "measures 32-bit integer throughput in GIOPS by executing chains of multiply and add operations on int or intN vectors in a kernel, dispatch size is determined automatically from device capabilities";
    }
};
