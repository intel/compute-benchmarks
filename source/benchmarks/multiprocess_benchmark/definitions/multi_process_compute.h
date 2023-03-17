/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/multi_device_selection_argument.h"
#include "framework/test_case/test_case.h"

struct MultiProcessComputeArguments : TestCaseArgumentContainer {
    MultipleTilesSelectionArgument deviceSelection;
    PositiveIntegerArgument processesPerTile;
    PositiveIntegerArgument workgroupsPerProcess;
    BooleanArgument synchronize;
    PositiveIntegerArgument operationsPerKernelCount;

    MultiProcessComputeArguments()
        : deviceSelection(*this, "tiles", "Tiles for execution"),
          processesPerTile(*this, "processesPerTile", "Number of processes that will be started on each of the tiles specified"),
          workgroupsPerProcess(*this, "workgroupsPerProcess", "Number of workgroups that each process will start"),
          synchronize(*this, "synchronize", "Synchronize all processes before each iteration"),
          operationsPerKernelCount(*this, "opsPerKernel", "Operations performed in kernel, used to steer its execution time") {}
};

struct MultiProcessCompute : TestCase<MultiProcessComputeArguments> {
    using TestCase<MultiProcessComputeArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiProcessCompute";
    }

    std::string getHelp() const override {
        return "Creates a number of separate processes for each tile specified performing a "
               "compute workload and measures average time to complete all of them. Processes "
               "will use affinity mask to select specific sub-devices for the execution";
    }
};
