/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EnqueueNdrTimeArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanArgument useOoq;
    BooleanArgument useProfiling;
    BooleanArgument useEvent;

    EnqueueNdrTimeArguments()
        : workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size"),
          useOoq(*this, "ooq", "Use out of order queue"),
          useProfiling(*this, "profiling", "Creating a profiling queue"),
          useEvent(*this, "event", "Pass output event to the enqueue call") {}
};

struct EnqueueNdrTime : TestCase<EnqueueNdrTimeArguments> {
    using TestCase<EnqueueNdrTimeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EnqueueNdrTime";
    }

    std::string getHelp() const override {
        return "measures time spent in clEnqueueNDRangeKernel on CPU.";
    }
};
