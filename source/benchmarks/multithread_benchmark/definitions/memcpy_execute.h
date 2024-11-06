/*
 * Copyright (C) 2024 Intel Corporation
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

    MemcpyExecuteArguments()
        : inOrderQueue(*this, "Ioq", "Create the queue with the in_order property"),
          numOpsPerThread(*this, "NumOpsPerThread", "Number of operations to execute on each thread"),
          numThreads(*this, "NumThreads", "Number of threads to use"),
          allocSize(*this, "AllocSize", "Size of the memory allocation in bytes"),
          measureCompletionTime(*this, "MeasureCompletion", "Measures time taken to complete the submissions (default is to measure only submit calls)"),
          useEvents(*this, "UseEvents", "Explicitly synchronize commands by events (needs to be set for Ioq=0)"),
          useQueuePerThread(*this, "UseQueuePerThread", "Use a separate queue in each thread"),
          srcUSM(*this, "SrcUSM", "Use USM for host source buffer"),
          dstUSM(*this, "DstUSM", "Use USM for host destination buffers") {}
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
