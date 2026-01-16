/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/priority_level_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct MultiQueueExecutionArguments : TestCaseArgumentContainer {
    PriorityLevelArgument measuredQueuePriority;
    PriorityLevelArgument secondaryQueuePriority;
    PositiveIntegerArgument kernelCount;
    BooleanArgument useSecondaryQueue;
    BooleanArgument useIoq;

    MultiQueueExecutionArguments()
        : measuredQueuePriority(*this, "measuredQueuePriority", "Priority level of the queue where kernels being measured are submitted"),
          secondaryQueuePriority(*this, "secondaryQueuePriority", "Priority level of the other queue submitting kernels to create contention"),
          kernelCount(*this, "kernelCount", "How many kernels to switch between"),
          useSecondaryQueue(*this, "useSecondaryQueue", "Use secondary cmdlist"),
          useIoq(*this, "useIoq", "Use implicit synchronization for kernel execution") {}
};

struct MultiQueueExecution : TestCase<MultiQueueExecutionArguments> {
    using TestCase<MultiQueueExecutionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiQueueExecution";
    }

    std::string getHelp() const override {
        return "submits empty kernels on queues with different priorities and measures total execution time of both queues.";
    }
};