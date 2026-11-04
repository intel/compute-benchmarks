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

struct UsmCopyBidirectionalReadArguments : PeerCopyArguments {
    EngineArgument localEngine;
    EngineArgument peerEngine;

    UsmCopyBidirectionalReadArguments()
        : localEngine(*this, "localEngine", "Engine of the local device, which reads from the peer"),
          peerEngine(*this, "peerEngine", "Engine of the peer device, which reads from the local device") {}
};

struct UsmCopyBidirectionalRead : TestCase<UsmCopyBidirectionalReadArguments> {
    using TestCase<UsmCopyBidirectionalReadArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyBidirectionalRead";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of two copies at "
               "the same time, each device reading from the other on an immediate command list. The copies are released "
               "together by one host-signalled event. Reports each copy and 'AllCopies', the bytes of both copies divided by "
               "the time until the last copy completes, which is always measured with the CPU timer.";
    }
};
