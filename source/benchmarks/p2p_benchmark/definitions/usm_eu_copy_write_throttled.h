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

struct UsmEuCopyWriteThrottledArguments : PeerCopyArguments {
    EngineArgument engine;
    PositiveIntegerArgument throttledWorkItems;

    UsmEuCopyWriteThrottledArguments()
        : engine(*this, "engine", "Compute engine of the local device, which writes to the peer"),
          throttledWorkItems(*this, "throttledWorkItems", "Number of work-items of each copy kernel, a multiple of 64") {}

    bool validateArgumentsExtra() const override {
        return PeerCopyArguments::validateArgumentsExtra() && isComputeEngine(engine) && isValidThrottle(throttledWorkItems);
    }
};

struct UsmEuCopyWriteThrottled : TestCase<UsmEuCopyWriteThrottledArguments> {
    using TestCase<UsmEuCopyWriteThrottledArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmEuCopyWriteThrottled";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of EU writes from "
               "the local device to the peer device. The copy kernel from p2p_benchmark_copy_throttled.cl copies the "
               "source buffer to the peer with throttledWorkItems work-items on a compute engine. The work-items loop "
               "over the buffer and after every iteration one thread per work-group pauses while the work-group waits on "
               "a barrier, so the work-item count sets the rate of the peer writes. Limiting the data in flight avoids "
               "the bandwidth collapse of concurrent peer writes on some systems. The best count depends on the system.";
    }
};
