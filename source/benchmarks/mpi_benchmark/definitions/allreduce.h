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

struct MpiAllReduceArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument numberOfRanks;
    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument sendType;
    UsmMemoryPlacementArgument recvType;

    MpiAllReduceArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          numberOfRanks(*this, "numberOfRanks", "Total number of MPI ranks"),
          messageSize(*this, "messageSize", "Size of the messages"),
          sendType(*this, "sendType", "Memory type of the MPI_Allreduce send buffer"),
          recvType(*this, "recvType", "Memory type of the MPI_Allreduce recv buffer") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        numberOfRanks = 1;
        messageSize = 1;
        sendType = UsmMemoryPlacement::Host;
        recvType = UsmMemoryPlacement::Host;
    }
};

struct MpiAllReduce : public TestCase<MpiAllReduceArguments> {
    using TestCase<MpiAllReduceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiAllReduce";
    }

    std::string getHelp() const override {
        return "Measures the performance of the all-reduction operation in an MPI+X application.";
    }
};
