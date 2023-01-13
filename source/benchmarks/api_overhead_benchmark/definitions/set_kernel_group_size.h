/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SetKernelGroupSizeArguments : TestCaseArgumentContainer {
    BooleanArgument asymmetricLocalWorkSize;

    SetKernelGroupSizeArguments()
        : asymmetricLocalWorkSize(*this, "asymmetricLocalWorkSize", "Use asymmetric local workSize") {}
};

struct SetKernelGroupSize : TestCase<SetKernelGroupSizeArguments> {
    using TestCase<SetKernelGroupSizeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SetKernelGroupSize";
    }

    std::string getHelp() const override {
        return "measures time spent in zeKernelSetGroupSize on CPU.";
    }
};
