/*
 * Copyright (C) 2024-2025 Intel Corporation
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
    BooleanArgument useGlobalIds;
    PositiveIntegerArgument count;
    BooleanArgument exec;
    PositiveIntegerArgument lws;
    PositiveIntegerArgument groupCount;
    BooleanArgument reverseOrder;
    BooleanArgument useL0NewArgApi;

    MultiArgumentKernelTimeArguments()
        : argumentCount(*this, "argumentCount", "argument count in a kernel, supported values 1,4,8,16,32,64"),
          measureSetKernelArg(*this, "measureSetArg", "control whether setKernelArgSvmPointer is measured or not"),
          useGlobalIds(*this, "ids", "control whether kernel uses global ids"),
          count(*this, "count", "how many kernel launches are measured"),
          exec(*this, "exec", "measure execute as well"),
          lws(*this, "lws", "local work size"),
          groupCount(*this, "groupCount", "total amount of work groups"),
          reverseOrder(*this, "reverseOrder", "set kernel arguments in reverse order"),
          useL0NewArgApi(*this, "newApi", "utilize zeCommandListAppendLaunchKernelWithArguments for L0 submissions") {}
};

struct MultiArgumentKernelTime : TestCase<MultiArgumentKernelTimeArguments> {
    using TestCase<MultiArgumentKernelTimeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiArgumentKernelTime";
    }

    std::string getHelp() const override {
        return "measures time spent in clEnqueueNDRangeKernel on CPU for kernels with multiple arguments.";
    }
};
