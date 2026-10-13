/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct GraphWithTransfersArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument withCopyOffload;
    BooleanArgument withComputeGraph;
    PositiveIntegerArgument computeIterations;

    GraphWithTransfersArguments()
        : size(*this, "size", "Size of the host and device buffers transferred and processed by the graph"),
          withCopyOffload(*this, "withCopyOffload", "Create the capturing immediate command list with the copy offload hint"),
          withComputeGraph(*this, "withComputeGraph", "Execute a second graph of 3 compute-bound kernels on a separate immediate command list in parallel with the transfer graph"),
          computeIterations(*this, "computeIterations", "Number of loop iterations in each compute-bound kernel of the compute graph") {}
};

struct GraphWithTransfers : TestCase<GraphWithTransfersArguments> {
    using TestCase<GraphWithTransfersArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "GraphWithTransfers";
    }

    std::string getHelp() const override {
        return "measures execution time of a graph captured on an immediate command list, consisting of a host-to-device copy, "
               "a kernel processing the device buffer and a device-to-host copy. Optionally, a second graph of 3 compute-bound kernels "
               "is executed in parallel on a separate immediate command list, competing for the same compute resources, "
               "and the measured time lasts until both graphs complete. Results are verified after benchmarking.";
    }
};
