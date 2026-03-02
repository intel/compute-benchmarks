/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelSubmitGraphVllmMockArguments : TestCaseArgumentContainer {
    Uint32Argument kernelWGCount;
    Uint32Argument kernelWGSize;
    Uint32Argument allocCount;
    Uint32Argument graphScenario;
    BooleanArgument useProfiling;
    BooleanArgument useEvents;

    KernelSubmitGraphVllmMockArguments() : kernelWGCount(*this, "KernelWGCount", "Number of workgroups."),
                                           kernelWGSize(*this, "KernelWGSize", "Size of each workgroup."),
                                           allocCount(*this, "AllocCount", "Number of allocations."),
                                           graphScenario(*this, "GraphScenario", "Graph scenario to run. 0: combined graphs 1-3, 1: layer normalization, 2: self-attention layer, 3: post attention layer normalization."),
                                           useProfiling(*this, "Profiling", "Create the queue with the profiling enabled."),
                                           useEvents(*this, "UseEvents", "Use SYCL events for kernel submissions (true) or eventless submit via nd_launch (false).") {}
};

struct KernelSubmitGraphVllmMock : TestCase<KernelSubmitGraphVllmMockArguments> {
    using TestCase::TestCase;

    std::string getTestCaseName() const override {
        return "KernelSubmitGraphVllmMock";
    }

    std::string getHelp() const override {
        return "Measures the overhead of SYCL graph submission using mock kernels that represent the computation of Llama-3.2-1B model"
               " with the exception that gemm kernel is not included."
               " The kernels do not perform any real computation but have similar launch parameters and resource usage as the actual kernels used in the model.";
    }
};
