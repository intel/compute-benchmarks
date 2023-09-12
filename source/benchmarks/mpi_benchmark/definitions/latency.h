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

struct MpiLatencyArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument sendType;
    UsmMemoryPlacementArgument recvType;

    MpiLatencyArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          messageSize(*this, "messageSize", "Size of the messages"),
          sendType(*this, "sendType", "Memory type of the MPI_Send buffer"),
          recvType(*this, "recvType", "Memory type of the MPI_Recv buffer") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        messageSize = 1;
        sendType = UsmMemoryPlacement::Host;
        recvType = UsmMemoryPlacement::Host;
    }
};

struct MpiLatency : public TestCase<MpiLatencyArguments> {
    using TestCase<MpiLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiLatency";
    }

    std::string getHelp() const override {
        return "Measures the point-to-point message transfer latency in an MPI+X application.";
    }
};
