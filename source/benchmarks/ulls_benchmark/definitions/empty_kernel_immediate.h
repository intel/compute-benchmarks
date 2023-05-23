/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EmptyKernelImmediateArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanFlagArgument useEventForHostSync;

    EmptyKernelImmediateArguments()
        : workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)"),
          useEventForHostSync(*this, "UseEventForHostSync",
                              "If true, use events to synchronize with host.If false, use zeCommandListHostSynchronize") {}
};

struct EmptyKernelImmediate : TestCase<EmptyKernelImmediateArguments> {
    using TestCase<EmptyKernelImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EmptyKernelImmediate";
    }

    std::string getHelp() const override {
        return "enqueues empty kernel and measures time to launch it using immediate command list and wait for it on CPU, thus "
               "measuring walker spawn time.";
    }
};
