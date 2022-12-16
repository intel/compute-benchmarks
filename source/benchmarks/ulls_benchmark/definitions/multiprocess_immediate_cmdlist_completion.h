/*
 * Copyright (C) 2023 Intel Corporation
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

struct MultiProcessImmediateCmdlistCompletionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfProcesses;
    PositiveIntegerArgument processesPerEngine;
    StringArgument engineGroup;
    PositiveIntegerArgument copySize;
    EngineMaskArgument engineMask;

    MultiProcessImmediateCmdlistCompletionArguments()
        : numberOfProcesses(*this, "numberOfProcesses", "total number of processes"),
          processesPerEngine(*this, "processesPerEngine", "number of processes submitting commands to each engine"),
          engineGroup(*this, "engineGroup", "engine group to be used"),
          copySize(*this, "copySize", "copy size in bytes "),
          engineMask(*this, "engineMask", "bit mask for selecting engines to be used for submission") {}
};

struct MultiProcessImmediateCmdlistCompletion : TestCase<MultiProcessImmediateCmdlistCompletionArguments> {
    using TestCase<MultiProcessImmediateCmdlistCompletionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiProcessImmediateCmdlistCompletion";
    }

    std::string getHelp() const override {
        return "measures completion latency of AppendMemoryCopy issued from multiple processes to Immediate Command Lists."
               "Engines to be used for submissions are selected based on the enabled bits of engineMask."
               "Bits of the 'engineMask' are indexed from right to left. So rightmost bit represents first engine and leftmost, the last engine."
               "'processesPerEngine' number of processes submits commands to each selected engine."
               "If 'numberOfProcesses' is greater than 'processesPerEngine' x selected engine count, then the excess processes are "
               "assigned to selected engines one each, in a round-robin method."
               "if selected engineCount == 1, then all processes are assigned to that engine.";
    }
};
