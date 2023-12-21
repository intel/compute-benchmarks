/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct MpiOverlapArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;

    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument sendType;
    UsmMemoryPlacementArgument recvType;
    BooleanArgument gpuCompute;
    IntegerArgument numMpiTestCalls;

    MpiOverlapArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          messageSize(*this, "messageSize", "Size of the messages"),
          sendType(*this, "sendType", "Memory type of the MPI_Send buffer"),
          recvType(*this, "recvType", "Memory type of the MPI_Recv buffer"),
          gpuCompute(*this, "gpuCompute", "Whether we do computation on the GPU or not"),
          numMpiTestCalls(*this, "numMpiTestCalls", "Number of MPI_Test calls during computation") {
        mpiLauncher = "mpirun";
        messageSize = 1;
        sendType = UsmMemoryPlacement::Host;
        recvType = UsmMemoryPlacement::Host;
        gpuCompute = false;
        numMpiTestCalls = 0;
    }
};

struct MpiOverlap : public TestCase<MpiOverlapArguments> {
    using TestCase<MpiOverlapArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiOverlap";
    }

    std::string getHelp() const override {
        return "Measures the computation-communication overlap efficency when using non-blocking message transfers in an MPI+X application.";
    }
};
