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

struct MpiReduceArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument numberOfRanks;
    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument sendType;
    UsmMemoryPlacementArgument recvType;

    MpiReduceArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          numberOfRanks(*this, "numberOfRanks", "Total number of MPI ranks"),
          messageSize(*this, "messageSize", "Size of the messages"),
          sendType(*this, "sendType", "Memory type of the MPI_Reduce send buffer"),
          recvType(*this, "recvType", "Memory type of the MPI_Reduce recv buffer") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        numberOfRanks = 1;
        messageSize = 1;
        sendType = UsmMemoryPlacement::Host;
        recvType = UsmMemoryPlacement::Host;
    }
};

struct MpiReduce : public TestCase<MpiReduceArguments> {
    using TestCase<MpiReduceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiReduce";
    }

    std::string getHelp() const override {
        return "Measures the performance of the reduction operation in an MPI+X application.";
    }
};
