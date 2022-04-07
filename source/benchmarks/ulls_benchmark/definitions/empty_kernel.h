/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EmptyKernelArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    EmptyKernelArguments()
        : workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)") {}
};

struct EmptyKernel : TestCase<EmptyKernelArguments> {
    using TestCase<EmptyKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EmptyKernel";
    }

    std::string getHelp() const override {
        return "enqueues empty kernel and measures time to launch it and wait for it on CPU, thus "
               "measuring walker spawn time.";
    }
};
