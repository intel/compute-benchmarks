/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct QueuePrioritiesArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument lowPriorityKernelTime;
    BooleanArgument usePriorities;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument highPriorityKernelTime;
    PositiveIntegerArgument sleepTime;

    QueuePrioritiesArguments()
        : lowPriorityKernelTime(*this, "lowTime", "How long each work item is in low priority kernel"),
          usePriorities(*this, "priorities", "Low priority command queue property is used"),
          workgroupCount(*this, "wgc", "Workgroup count of high priority kernel"),
          highPriorityKernelTime(*this, "highTime", "How long each work item is in high priority kernel"),
          sleepTime(*this, "sleep", "sleep time in us after low priority kernel flushed") {}
};

struct QueuePriorities : TestCase<QueuePrioritiesArguments> {
    using TestCase<QueuePrioritiesArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "QueuePriorities";
    }

    std::string getHelp() const override {
        return "Uses queues with different priorities to meassure submission and context switch latencies";
    }
};
