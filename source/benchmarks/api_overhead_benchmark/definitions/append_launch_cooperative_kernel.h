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

struct AppendLaunchCooperativeKernel : TestCase<AppendLaunchKernelArguments> {
    using TestCase<AppendLaunchKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendLaunchCooperativeKernel";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendLaunchCooperativeKernel on CPU.";
    }
};
