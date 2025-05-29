/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/tmp_memory_strategy_argument.h"
#include "framework/test_case/test_case.h"

struct TmpBufferMixedSizeArguments : TestCaseArgumentContainer {
    ByteSizeArgument sizeSmall;
    PositiveIntegerArgument sizeLargeRatio;
    TmpMemoryStrategyArgument strategy;
    PositiveIntegerArgument numKernelsSmall;

    TmpBufferMixedSizeArguments()
        : sizeSmall(*this, "sizeSmall", "Small buffer size in bytes"),
          sizeLargeRatio(*this, "sizeLarge", "Multiplier for large buffer size"),
          strategy(*this, "strategy", "Strategy used to allocate temporary buffers"),
          numKernelsSmall(*this, "numKernelsSmall", "Number of kernels executed on a small buffer per iteration") {}
};

struct TmpBufferMixedSize : TestCase<TmpBufferMixedSizeArguments> {
    using TestCase<TmpBufferMixedSizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "TmpBufferMixedSize";
    }

    std::string getHelp() const override {
        return "measures time used to allocate temporary buffers and use them in a kernel, using two queues.";
    }
};
