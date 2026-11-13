/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include "definitions/peer_copy_arguments.h"

#include <optional>

struct UsmCopyBidirectionalWriteMultipleBlitsArguments : PeerCopyArguments {
    PeerCopyBlittersArgument localBlitters;
    PeerCopyBlittersArgument peerBlitters;

    UsmCopyBidirectionalWriteMultipleBlitsArguments()
        : localBlitters(*this, "localBlitters", "Bit mask of the copy engines of the local device, which write to the peer. The rightmost bit is BCS0"),
          peerBlitters(*this, "peerBlitters", "Bit mask of the copy engines of the peer device, which write to the local device. The rightmost bit is BCS0") {}
};

struct UsmCopyBidirectionalWriteMultipleBlits : TestCase<UsmCopyBidirectionalWriteMultipleBlitsArguments> {
    using TestCase<UsmCopyBidirectionalWriteMultipleBlitsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyBidirectionalWriteMultipleBlits";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of two copies at the same time, each device writing to the other on one immediate command list per selected copy engine. All engines are released together by one host-signalled event. The copy of each device is split across the selected copy engines: with the main copy engine (the rightmost bit) and link copy engines the main copy engine copies half of the size and the link copy engines share the other half, otherwise the engines share the size equally. At least one of the two masks must select two or more engines. Reports each engine, each copy as the time from the first engine start to the last engine end, and 'AllCopies' when there are two copies, the bytes of both copies divided by the time until the last copy completes, which is always measured with the CPU timer.";
    }
};
