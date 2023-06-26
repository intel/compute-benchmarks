/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSetArgumentValueImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument argumentSize;
    BooleanArgument differentValues;

    KernelSetArgumentValueImmediateArguments()
        : argumentSize(*this, "argSize", "Kernel argument size in bytes"),
          differentValues(*this, "differentValues", "Use different values for arguments each iteration") {}
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
