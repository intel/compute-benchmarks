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
    IntegerArgument numQueuesPerThread;
    PositiveIntegerArgument numThreads;
    PositiveIntegerArgument allocSize;
    BooleanArgument measureCompletionTime;
    BooleanArgument useEvents;

    MemcpyExecuteArguments()
        : inOrderQueue(*this, "Ioq", "Create the queue with the in_order property"),
          numQueuesPerThread(*this, "NumQueuesPerThread", "Number of queues to use per thread, if 0 then all threads use the same queue"),
          numThreads(*this, "NumThreads", "Number of threads to use"),
          allocSize(*this, "AllocSize", "Size of the memory allocation in bytes"),
          measureCompletionTime(*this, "MeasureCompletion", "Measures time taken to complete the submissions (default is to measure only submit calls)"),
          useEvents(*this, "UseEvents", "Explicitly synchronize commands by events (needs to be set for Ioq=0)") {}
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
