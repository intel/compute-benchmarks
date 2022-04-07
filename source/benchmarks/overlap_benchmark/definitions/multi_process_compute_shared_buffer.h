/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/multi_device_selection_argument.h"
#include "framework/test_case/test_case.h"

struct MultiProcessComputeSharedBufferArguments : TestCaseArgumentContainer {
    MultipleTilesSelectionArgument deviceSelection;
    PositiveIntegerArgument processesPerTile;
    PositiveIntegerArgument workgroupsPerProcess;
    BooleanArgument synchronize;

    MultiProcessComputeSharedBufferArguments()
        : deviceSelection(*this, "tiles", "Tiles for execution"),
          processesPerTile(*this, "processesPerTile", "Number of processes that will be started on each of the tiles specified"),
          workgroupsPerProcess(*this, "workgroupsPerProcess", "Number of workgroups that each process will start"),
          synchronize(*this, "synchronize", "Synchronize all processes before each iteration") {}
};

struct MultiProcessComputeSharedBuffer : TestCase<MultiProcessComputeSharedBufferArguments> {
    using TestCase<MultiProcessComputeSharedBufferArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiProcessComputeSharedBuffer";
    }

    std::string getHelp() const override {
        return "Creates a number of separate processes for each tile specified performing a "
               "compute workload and measures average time to complete all of them. Processes "
               "will use affinity mask to select specific sub-devices for the execution. A single "
               "buffer for each tile is created by parent process. All processes executing on a "
               "given tile will share it via IPC calls. ";
    }
};
