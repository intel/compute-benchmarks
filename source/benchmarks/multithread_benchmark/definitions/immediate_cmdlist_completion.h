/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/bitmap_argument.h"
#include "framework/test_case/test_case.h"

constexpr uint32_t maxNumberOfEngines = 32;
using EngineMaskArgument = BitmaskArgument<maxNumberOfEngines, false>;

struct ImmediateCommandListCompletionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfThreads;
    PositiveIntegerArgument threadsPerEngine;
    StringArgument engineGroup;
    PositiveIntegerArgument copySize;
    EngineMaskArgument engineMask;
    BooleanArgument withCopyOffload;

    ImmediateCommandListCompletionArguments()
        : numberOfThreads(*this, "numberOfThreads", "total number of threads"),
          threadsPerEngine(*this, "threadsPerEngine", "number of threads submitting commands to each engine"),
          engineGroup(*this, "engineGroup", "engine group to be used"),
          copySize(*this, "copySize", "copy size in bytes "),
          engineMask(*this, "engineMask", "bit mask for selecting engines to be used for submission"),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)") {}
};

struct ImmediateCommandListCompletion : TestCase<ImmediateCommandListCompletionArguments> {
    using TestCase<ImmediateCommandListCompletionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ImmediateCommandListCompletion";
    }

    std::string getHelp() const override {
        return "measures completion latency of AppendMemoryCopy issued from multiple threads to Immediate Command Lists."
               "Engines to be used for submissions are selected based on the enabled bits of engineMask."
               "'threadsPerEngine' number of threads submits commands to each selected engine."
               "If 'numberOfThreads' is greater than 'threadsPerEngine' x selected engine count, then the excess threads are "
               "assigned to selected engines one each, in a round-robin method."
               "if selected engineCount == 1, then all threads are assigned to that engine.";
    }
};
