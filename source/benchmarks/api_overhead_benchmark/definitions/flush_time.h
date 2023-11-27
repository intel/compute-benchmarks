/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct FlushTimeArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupCount;
    IntegerArgument workgroupSize;
    BooleanArgument useOoq;
    BooleanArgument useEvent;
    PositiveIntegerArgument flushCount;

    FlushTimeArguments()
        : workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size, pass 0 to make the driver calculate it during enqueue"),
          useOoq(*this, "ooq", "Use out of order queue"),
          useEvent(*this, "event", "Pass output event to the enqueue call"),
          flushCount(*this, "flushCount", "Count of flushes to measure") {}
};

struct FlushTime : TestCase<FlushTimeArguments> {
    using TestCase<FlushTimeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "FlushTime";
    }

    std::string getHelp() const override {
        return "measures time spent in clEnqueueNDRangeKernel on CPU.";
    }
};
