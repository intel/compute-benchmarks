/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MemcpyExecuteArguments : TestCaseArgumentContainer {
    BooleanArgument inOrderQueue;
    PositiveIntegerArgument numOpsPerThread;
    PositiveIntegerArgument numThreads;
    PositiveIntegerArgument allocSize;
    BooleanArgument measureCompletionTime;
    BooleanArgument useEvents;
    BooleanArgument useQueuePerThread;
    BooleanArgument srcUSM;
    BooleanArgument dstUSM;
    BooleanArgument useBarrier;

    MemcpyExecuteArguments()
        : inOrderQueue(*this, "Ioq", "Create the queue with the in_order property"),
          numOpsPerThread(*this, "NumOpsPerThread", "Number of operations to execute on each thread"),
          numThreads(*this, "NumThreads", "Number of threads to use"),
          allocSize(*this, "AllocSize", "Size of the memory allocation in bytes"),
          measureCompletionTime(*this, "MeasureCompletion", "Measures time taken to complete the submissions (default is to measure only submit calls)"),
          useEvents(*this, "UseEvents", "Explicitly synchronize commands by events (needs to be set for Ioq=0)"),
          useQueuePerThread(*this, "UseQueuePerThread", "Use a separate queue in each thread"),
          srcUSM(*this, "SrcUSM", "Use USM for host source buffer"),
          dstUSM(*this, "DstUSM", "Use USM for host destination buffers"),
          useBarrier(*this, "UseBarrier", "Submit barrier after each iteration (SYCL-only)") {}
};

struct MemcpyExecute : TestCase<MemcpyExecuteArguments> {
    using TestCase<MemcpyExecuteArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MemcpyExecute";
    }

    std::string getHelp() const override {
        return "measures time spent exeucting kernels interleved with memcpy operations";
    }
};

// verify the results
static inline TestResult verifyResults(size_t numThreads, size_t numOpsPerThread, size_t allocSize, std::vector<void *> &dst_buffers, int value) {
    for (size_t t = 0; t < numThreads; t++) {
        for (size_t i = 0; i < numOpsPerThread; i++) {
            for (size_t j = 0; j < allocSize / sizeof(int); j++) {
                auto v = *(((char *)dst_buffers[t]) + i * allocSize + j * sizeof(int));
                if (v != value) {
                    std::cerr << "dst_buffers at: " << t << " " << i << " " << j << " , is: " << (int)v << std::endl;
                    return TestResult::Error;
                }
            }
        }
    }
    return TestResult::Success;
}
