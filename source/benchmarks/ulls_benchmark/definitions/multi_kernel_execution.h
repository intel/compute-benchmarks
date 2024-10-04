/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct MultiKernelExecutionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelCount;
    PositiveIntegerArgument workgroupSize;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument delay;
    BooleanArgument inOrderOverOOO;
    BooleanArgument profiling;

    MultiKernelExecutionArguments()
        : kernelCount(*this, "count", "Count of kernels within command list"),
          workgroupSize(*this, "wkgSizes", "work group size of each kernel"),
          workgroupCount(*this, "wkgCount", "work group count of each kernel"),
          delay(*this, "delay", "how much delay between atomic reads"),
          inOrderOverOOO(*this, "inOrderOverOOO", "use out of order queue to implement in order queue"),
          profiling(*this, "profiling", "use profiling on the gpu to collect score") {}
};

struct MultiKernelExecution : TestCase<MultiKernelExecutionArguments> {
    using TestCase<MultiKernelExecutionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiKernelExecution";
    }

    std::string getHelp() const override {
        return "submits multiple kernel in single command lists, measures total time of their execution";
    }
};