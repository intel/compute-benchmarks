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

struct MpiBandwidthArguments : TestCaseArgumentContainer {
    StringArgument mpiLauncher;
    MpiStatisticsTypeArgument statsType;

    PositiveIntegerArgument messageSize;
    UsmMemoryPlacementArgument sendType;
    UsmMemoryPlacementArgument recvType;
    PositiveIntegerArgument nBatches;

    MpiBandwidthArguments()
        : mpiLauncher(*this, "mpiLauncher", "MPI launcher (e.g. mpirun --hostfile /path/to/hosts"),
          statsType(*this, "statsType", "MPI statistics type (Avg/Max/Min)"),
          messageSize(*this, "messageSize", "Size of the messages"),
          sendType(*this, "sendType", "Memory type of the MPI_Send buffer"),
          recvType(*this, "recvType", "Memory type of the MPI_Recv buffer"),
          nBatches(*this, "nBatches", "Number of message batches to transfer") {
        mpiLauncher = "mpirun";
        statsType = MpiStatisticsType::Avg;
        messageSize = 1;
        sendType = UsmMemoryPlacement::Host;
        recvType = UsmMemoryPlacement::Host;
        nBatches = 200;
    }
};

struct MpiBandwidth : public TestCase<MpiBandwidthArguments> {
    using TestCase<MpiBandwidthArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MpiBandwidth";
    }

    std::string getHelp() const override {
        return "Measures the point-to-point uni-directional message passing bandwidth in an MPI+X application.";
    }
};
