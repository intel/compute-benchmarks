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

struct UsmEuCopyWriteChunkedArguments : PeerCopyArguments {
    EngineArgument engine;
    ByteSizeArgument chunkSize;

    UsmEuCopyWriteChunkedArguments()
        : engine(*this, "engine", "Compute engine of the local device, which writes to the peer"),
          chunkSize(*this, "chunkSize", "Size of each chunk") {}

    bool validateArgumentsExtra() const override {
        return PeerCopyArguments::validateArgumentsExtra() && isComputeEngine(engine) && isValidReduceCopySize(chunkSize);
    }
};

struct UsmEuCopyWriteChunked : TestCase<UsmEuCopyWriteChunkedArguments> {
    using TestCase<UsmEuCopyWriteChunkedArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEuCopyWriteChunked";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of EU writes from "
               "the local device to the peer device. The copy kernel from p2p_benchmark_reduce_copy.cl copies the source "
               "buffer to the peer. It is launched once per chunk of chunkSize on an in-order immediate command list of a "
               "compute engine, so the peer writes in flight are limited to about one chunk. Limiting the data in flight "
               "avoids the bandwidth collapse of concurrent peer writes on some systems. With useEvents the time of a "
               "copy is measured from the start of its first chunk to the end of its last chunk.";
    }
};
