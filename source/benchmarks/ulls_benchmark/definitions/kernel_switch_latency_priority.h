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

struct KernelSwitchPriorityArguments : TestCaseArgumentContainer {
    PriorityLevelArgument measuredQueuePriority;
    PriorityLevelArgument secondaryQueuePriority;
    PositiveIntegerArgument kernelCount;
    PositiveIntegerArgument kernelExecutionTime;
    BooleanArgument useIoq;

    KernelSwitchPriorityArguments()
        : measuredQueuePriority(*this, "measuredQueuePriority", "Priority level of the queue where kernels being measured are submitted"),
          secondaryQueuePriority(*this, "secondaryQueuePriority", "Priority level of the other queue submitting kernels to create contention"),
          kernelCount(*this, "kernelCount", "How many kernels to switch between"),
          kernelExecutionTime(*this, "kernelExecutionTime", "Time for each kernel execution"),
          useIoq(*this, "useIoq", "Use implicit synchronization for kernel execution") {}
};

struct KernelSwitchPriority : TestCase<KernelSwitchPriorityArguments> {
    using TestCase<KernelSwitchPriorityArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSwitchPriority";
    }

    std::string getHelp() const override {
        return "submits kernels on queues with different priorities and measures kernel switch latency on single queue.";
    }
};