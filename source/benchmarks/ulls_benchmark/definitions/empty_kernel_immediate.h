/*
 * Copyright (C) 2022 Intel Corporation
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

    EmptyKernelImmediateArguments()
        : workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)") {}
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
