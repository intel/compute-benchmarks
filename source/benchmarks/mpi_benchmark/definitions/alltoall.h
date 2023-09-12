/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/mpi_statistics_type_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct MpiAllToAllArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument numberOfRanks;
    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument sendType;
    UsmMemoryPlacementArgument recvType;

    MpiAllToAllArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          numberOfRanks(*this, "numberOfRanks", "Total number of MPI ranks"),
          messageSize(*this, "messageSize", "Size of the messages"),
          sendType(*this, "sendType", "Memory type of the MPI_Alltoall send buffer"),
          recvType(*this, "recvType", "Memory type of the MPI_Alltoall recv buffer") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        numberOfRanks = 1;
        messageSize = 1;
        sendType = UsmMemoryPlacement::Host;
        recvType = UsmMemoryPlacement::Host;
    }
};

struct MpiAllToAll : public TestCase<MpiAllToAllArguments> {
    using TestCase<MpiAllToAllArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiAllToAll";
    }

    std::string getHelp() const override {
        return "Measures the performance of the all-to-all exchange in an MPI+X application.";
    }
};
