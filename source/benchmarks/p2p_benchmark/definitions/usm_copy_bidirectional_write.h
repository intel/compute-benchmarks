/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"

#include "definitions/peer_copy_arguments.h"

#include <optional>

struct UsmCopyBidirectionalWriteArguments : PeerCopyArguments {
    EngineArgument localEngine;
    EngineArgument peerEngine;

    UsmCopyBidirectionalWriteArguments()
        : localEngine(*this, "localEngine", "Engine of the local device, which writes to the peer"),
          peerEngine(*this, "peerEngine", "Engine of the peer device, which writes to the local device") {}
};

struct UsmCopyBidirectionalWrite : TestCase<UsmCopyBidirectionalWriteArguments> {
    using TestCase<UsmCopyBidirectionalWriteArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyBidirectionalWrite";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of two copies at "
               "the same time, each device writing to the other on an immediate command list. The copies are released "
               "together by one host-signalled event. Reports each copy and 'AllCopies', the bytes of both copies divided by "
               "the time until the last copy completes, which is always measured with the CPU timer.";
    }
};
