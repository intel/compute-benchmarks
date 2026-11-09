/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include "definitions/peer_copy_pairs_arguments.h"

#include <optional>

struct UsmCopyConcurrentPairsReadArguments : PeerCopyPairsArguments {};

struct UsmCopyConcurrentPairsRead : TestCase<UsmCopyConcurrentPairsReadArguments> {
    using TestCase<UsmCopyConcurrentPairsReadArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyConcurrentPairsRead";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on the root devices of every pair and measures the bandwidth of one copy per pair, all at the same time, in which the local device reads from the peer device on an immediate command list. Each copy runs on its own engine of the device that submits it. "
               "The copies are released together by one host-signalled event. Reports each copy and 'AllPairs', the bytes of all copies "
               "divided by the time until the last copy completes, which is always measured with the CPU timer. "
               "A device pair given in both orders, for example 0:1_1:0_2:3_3:2, copies in both directions.";
    }
};
