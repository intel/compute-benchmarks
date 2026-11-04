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

struct UsmEuCopyBidirectionalWriteThrottledArguments : PeerCopyArguments {
    EngineArgument localEngine;
    EngineArgument peerEngine;
    PositiveIntegerArgument throttledWorkItems;

    UsmEuCopyBidirectionalWriteThrottledArguments()
        : localEngine(*this, "localEngine", "Compute engine of the local device, which writes to the peer"),
          peerEngine(*this, "peerEngine", "Compute engine of the peer device, which writes to the local device"),
          throttledWorkItems(*this, "throttledWorkItems", "Number of work-items of each copy kernel, a multiple of 64") {}

    bool validateArgumentsExtra() const override {
        return PeerCopyArguments::validateArgumentsExtra() && isComputeEngine(localEngine) && isComputeEngine(peerEngine) && isValidThrottle(throttledWorkItems);
    }
};

struct UsmEuCopyBidirectionalWriteThrottled : TestCase<UsmEuCopyBidirectionalWriteThrottledArguments> {
    using TestCase<UsmEuCopyBidirectionalWriteThrottledArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEuCopyBidirectionalWriteThrottled";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of EU writes in "
               "both directions at the same time, each device writing to the other. The copy kernel from "
               "p2p_benchmark_copy_throttled.cl copies the source buffer to the peer with throttledWorkItems work-items "
               "on a compute engine. The work-items loop over the buffer and after every iteration one thread per "
               "work-group pauses while the work-group waits on a barrier, so the work-item count sets the rate of the "
               "peer writes. Limiting the data in flight avoids the bandwidth collapse of concurrent peer writes on some "
               "systems. The best count depends on the system. The copies are released together by one host-signalled "
               "event. Reports each copy and 'AllCopies', the bytes of both copies divided by the time until the last copy "
               "completes, which is always measured with the CPU timer.";
    }
};
