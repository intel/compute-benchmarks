/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"

#include "definitions/peer_copy_arguments.h"

#include <optional>

struct UsmEuReduceCopyBidirectionalWriteChunkedArguments : PeerCopyArguments {
    EngineArgument localEngine;
    EngineArgument peerEngine;
    ByteSizeArgument chunkSize;

    UsmEuReduceCopyBidirectionalWriteChunkedArguments()
        : localEngine(*this, "localEngine", "Compute engine of the local device, which writes to the peer"),
          peerEngine(*this, "peerEngine", "Compute engine of the peer device, which writes to the local device"),
          chunkSize(*this, "chunkSize", "Size of each chunk") {}

    bool validateArgumentsExtra() const override {
        return PeerCopyArguments::validateArgumentsExtra() && isComputeEngine(localEngine) && isComputeEngine(peerEngine) && isValidReduceCopySize(chunkSize);
    }
};

struct UsmEuReduceCopyBidirectionalWriteChunked : TestCase<UsmEuReduceCopyBidirectionalWriteChunkedArguments> {
    using TestCase<UsmEuReduceCopyBidirectionalWriteChunkedArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEuReduceCopyBidirectionalWriteChunked";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of EU writes in "
               "both directions at the same time, each device writing to the other. The reduce-copy kernel from "
               "p2p_benchmark_reduce_copy.cl computes out1 = in1 + in2 in local device memory and writes out1 to the "
               "peer. It is launched once per chunk of chunkSize on an in-order immediate command list of a compute "
               "engine, so the peer writes in flight are limited to about one chunk. Limiting the data in flight avoids "
               "the bandwidth collapse of concurrent peer writes on some systems. With useEvents the time of a copy is "
               "measured from the start of its first chunk to the end of its last chunk. The copies are released together "
               "by one host-signalled event. Reports each copy and 'AllCopies', the bytes of both copies divided by the time "
               "until the last copy completes, which is always measured with the CPU timer.";
    }
};
