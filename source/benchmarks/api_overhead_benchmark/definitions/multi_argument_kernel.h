/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MultiArgumentKernelTimeArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument argumentCount;
    BooleanArgument measureSetKernelArg;

    MultiArgumentKernelTimeArguments()
        : argumentCount(*this, "argumentCount", "argument count in a kernel, supported values 1,4,8,16,32,64"),
          measureSetKernelArg(*this, "measureSetArg", "control whether setKernelArgSvmPointer is measured or not") {}
};

struct MultiArgumentKernelTime : TestCase<MultiArgumentKernelTimeArguments> {
    using TestCase<MultiArgumentKernelTimeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiArgumentKernelTime";
    }

    std::string getHelp() const override {
        return "measures time spent in clEnqueueNDRangeKernel on CPU for kernels with mulitple arguments.";
    }
};
