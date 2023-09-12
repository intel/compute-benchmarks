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

struct MpiBcastArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument numberOfRanks;
    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument bufferType;

    MpiBcastArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          numberOfRanks(*this, "numberOfRanks", "Total number of MPI ranks"),
          messageSize(*this, "messageSize", "Size of the messages"),
          bufferType(*this, "recvType", "Memory type of the MPI_Bcast buffer") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        numberOfRanks = 1;
        messageSize = 1;
        bufferType = UsmMemoryPlacement::Host;
    }
};

struct MpiBcast : public TestCase<MpiBcastArguments> {
    using TestCase<MpiBcastArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiBcast";
    }

    std::string getHelp() const override {
        return "Measures the performance of the broadcast operation in an MPI+X application.";
    }
};
