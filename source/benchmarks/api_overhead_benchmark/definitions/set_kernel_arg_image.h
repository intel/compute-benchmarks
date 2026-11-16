/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSetArgumentValueImageArguments : TestCaseArgumentContainer {
    Uint32Argument argumentIndex;
    BooleanArgument differentValues;

    KernelSetArgumentValueImageArguments()
        : argumentIndex(*this, "argIndex", "Index of the kernel argument to set, from 0 to 7"),
          differentValues(*this, "differentValues", "Alternate between two images on every call") {}
};

struct KernelSetArgumentValueImage : TestCase<KernelSetArgumentValueImageArguments> {
    using TestCase<KernelSetArgumentValueImageArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSetArgumentValueImage";
    }

    std::string getHelp() const override {
        return "measures average CPU time of one zeKernelSetArgumentValue call for an image argument, over a loop of 100 calls per sample.";
    }
};
