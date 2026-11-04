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

struct UsmCopyWriteReadArguments : PeerCopyArguments {
    EngineArgument writeEngine;
    EngineArgument readEngine;

    UsmCopyWriteReadArguments()
        : writeEngine(*this, "writeEngine", "Engine of the local device that writes to the peer"),
          readEngine(*this, "readEngine", "Engine of the local device that reads from the peer. Must differ from writeEngine") {}

    bool validateArgumentsExtra() const override {
        return PeerCopyArguments::validateArgumentsExtra() && writeEngine != readEngine;
    }
};

struct UsmCopyWriteRead : TestCase<UsmCopyWriteReadArguments> {
    using TestCase<UsmCopyWriteReadArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyWriteRead";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of two copies at "
               "the same time, both submitted by the local device on two engines with immediate command lists: one writes "
               "to the peer and one reads from the peer, so the link carries data in both directions. The copies are "
               "released together by one host-signalled event. Reports each copy and 'AllCopies', the bytes of both copies "
               "divided by the time until the last copy completes, which is always measured with the CPU timer.";
    }
};
