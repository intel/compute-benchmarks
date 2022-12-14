/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ImmediateCommandListSubmissionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfThreads;
    PositiveIntegerArgument threadsPerEngine;

    ImmediateCommandListSubmissionArguments()
        : numberOfThreads(*this, "numberOfThreads", "total number of threads"),
          threadsPerEngine(*this, "threadsPerEngine", "number of threads submitting commands to each engine") {}
};

struct ImmediateCommandListSubmission : TestCase<ImmediateCommandListSubmissionArguments> {
    using TestCase<ImmediateCommandListSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ImmediateCommandListSubmission";
    }

    std::string getHelp() const override {
        return "measures submission latency of AppendLaunchKernel issued from multiple threads to Immediate Command Lists."
               "'threadsPerEngine' count of threads submit commands to each engine."
               "If 'numberOfThreads' is greater than 'threadsPerEngine' x engine count, then the excess threads are "
               "assigned to engines one each, in a round-robin method."
               "if engineCount == 1, then all threads are assigned to the engine.";
    }
};
