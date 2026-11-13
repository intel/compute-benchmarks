/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct KernelIntegerExecutionArguments : TestCaseArgumentContainer {
    UsmRuntimeMemoryPlacementArgument placement;

    KernelIntegerExecutionArguments()
        : placement(*this, "placement", "Placement of the integer incremented by the kernel") {}
};

struct KernelIntegerExecution : TestCase<KernelIntegerExecutionArguments> {
    using TestCase<KernelIntegerExecutionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelIntegerExecution";
    }

    std::string getHelp() const override {
        return "enqueues a single work item kernel which increments an integer and measures time to execute it and "
               "synchronize the queue on CPU.";
    }
};
