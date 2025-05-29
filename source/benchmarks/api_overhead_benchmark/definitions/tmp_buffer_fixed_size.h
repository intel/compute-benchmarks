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

struct TmpBufferFixedSizeArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    TmpMemoryStrategyArgument strategy;
    PositiveIntegerArgument numKernels;

    TmpBufferFixedSizeArguments()
        : size(*this, "size", "Buffer size in bytes"),
          strategy(*this, "strategy", "Strategy used to allocate temporary buffers"),
          numKernels(*this, "numKernels", "Number of kernels to submit in a single iteration") {}
};

struct TmpBufferFixedSize : TestCase<TmpBufferFixedSizeArguments> {
    using TestCase<TmpBufferFixedSizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "TmpBufferFixedSize";
    }

    std::string getHelp() const override {
        return "measures time used to allocate temporary buffer and use it in a kernel.";
    }
};
