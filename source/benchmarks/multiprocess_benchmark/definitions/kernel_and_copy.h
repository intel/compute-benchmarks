/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct KernelAndCopyArguments : TestCaseArgumentContainer {
    BooleanArgument twoQueues;
    BooleanArgument runKernel;
    BooleanArgument runCopy;
    BooleanArgument useCopyQueue;

    KernelAndCopyArguments()
        : twoQueues(*this, "twoQueues", "Enables using separate queues for both operations. Must be used with runCopy and runKernel"),
          runKernel(*this, "runKernel", "Enqueue kernel during each iteration"),
          runCopy(*this, "runCopy", "Enqueue buffer to buffer copy during each iteration"),
          useCopyQueue(*this, "useCopyQueue", "Use a specialized copy queue for the copy operation. Must be used with runCopy") {}

    bool validateArgumentsExtra() const override {
        if (!runKernel && !runCopy) {
            return false;
        }
        if ((!runCopy || !runKernel) && twoQueues) {
            return false;
        }
        if (!runCopy && useCopyQueue) {
            return false;
        }
        if (runCopy && runKernel && !twoQueues && useCopyQueue) {
            return false;
        }
        return true;
    }
};

struct KernelAndCopy : TestCase<KernelAndCopyArguments> {
    using TestCase<KernelAndCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelAndCopy";
    }

    std::string getHelp() const override {
        return "enqueues kernel and copy operation with the ability to perform both tasks on "
               "different command queues.";
    }
};
