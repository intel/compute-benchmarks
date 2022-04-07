/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSetArgumentValueImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument argumentSize;

    KernelSetArgumentValueImmediateArguments()
        : argumentSize(*this, "argSize", "Kernel argument size in bytes") {}
};

struct KernelSetArgumentValueImmediate : TestCase<KernelSetArgumentValueImmediateArguments> {
    using TestCase<KernelSetArgumentValueImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSetArgumentValueImmediate";
    }

    std::string getHelp() const override {
        return "measures time spent in zeKernelSetArgumentValue for immediate arguments on CPU.";
    }
};
