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

struct UsmCopyPushPullArguments : PeerCopyArguments {
    EngineArgument writeEngine;
    EngineArgument readEngine;

    UsmCopyPushPullArguments()
        : writeEngine(*this, "writeEngine", "Engine of the local device, which writes to the peer"),
          readEngine(*this, "readEngine", "Engine of the peer device, which reads from the local device") {}
};

struct UsmCopyPushPull : TestCase<UsmCopyPushPullArguments> {
    using TestCase<UsmCopyPushPullArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyPushPull";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of two copies at "
               "the same time on immediate command lists: the local device writes to the peer while the peer reads from "
               "the local device, so both copies move data in the same direction. The copies are released together by one "
               "host-signalled event. Reports each copy and 'AllCopies', the bytes of both copies divided by the time until "
               "the last copy completes, which is always measured with the CPU timer.";
    }
};
