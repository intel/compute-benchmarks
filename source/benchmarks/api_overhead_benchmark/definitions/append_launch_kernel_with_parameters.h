/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

#include "append_launch_kernel.h"

struct AppendLaunchKernelWithParametersArguments : AppendLaunchKernelArguments {
    BooleanArgument useCooperative;

    AppendLaunchKernelWithParametersArguments()
        : useCooperative(*this, "cooperative", "Pass cooperative descriptor via pNext") {}
};

struct AppendLaunchKernelWithParameters : TestCase<AppendLaunchKernelWithParametersArguments> {
    using TestCase<AppendLaunchKernelWithParametersArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendLaunchKernelWithParameters";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendLaunchKernelWithParameters on CPU.";
    }
};
