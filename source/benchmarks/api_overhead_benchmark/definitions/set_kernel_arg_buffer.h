/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSetArgumentValueBufferArguments : TestCaseArgumentContainer {
    Uint32Argument argumentIndex;
    BooleanArgument differentValues;

    KernelSetArgumentValueBufferArguments()
        : argumentIndex(*this, "argIndex", "Index of the kernel argument to set, from 0 to 7"),
          differentValues(*this, "differentValues", "Alternate between two buffers on every call") {}
};

struct KernelSetArgumentValueBuffer : TestCase<KernelSetArgumentValueBufferArguments> {
    using TestCase<KernelSetArgumentValueBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSetArgumentValueBuffer";
    }

    std::string getHelp() const override {
        return "measures average CPU time of one zeKernelSetArgumentValue call for a device USM buffer argument, over a loop of 100 calls per sample.";
    }
};
