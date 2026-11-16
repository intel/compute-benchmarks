/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/data_type_argument.h"
#include "framework/test_case/test_case.h"

#include <cstdint>

struct BlackScholesArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;

    BlackScholesArguments()
        : dataType(*this, "dataType", "Floating point type of the options and results, only Float and Double are supported") {}
};

struct BlackScholes : TestCase<BlackScholesArguments> {
    using TestCase<BlackScholesArguments>::TestCase;

    static constexpr uint32_t optionCount = 1024u * 1024u;
    static constexpr uint32_t groupSize = 256u;
    static constexpr uint32_t kernelLaunchCount = 50u;
    static constexpr double riskFree = 0.02;
    static constexpr double volatility = 0.3;

    std::string getTestCaseName() const override {
        return "BlackScholes";
    }

    std::string getHelp() const override {
        return "measures the time of one workload execution: copies three arrays of 1M generated options to the device, launches a kernel that prices call and put options with the Black-Scholes formula 50 times and copies both results to the host";
    }
};
