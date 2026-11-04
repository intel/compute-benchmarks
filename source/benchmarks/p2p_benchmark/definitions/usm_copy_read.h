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

struct UsmCopyReadArguments : PeerCopyArguments {
    EngineArgument engine;

    UsmCopyReadArguments()
        : engine(*this, "engine", "Engine of the local device, which reads from the peer") {}
};

struct UsmCopyRead : TestCase<UsmCopyReadArguments> {
    using TestCase<UsmCopyReadArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyRead";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of a copy from the "
               "peer device to the local device, submitted by the local device on an immediate command list, so the local "
               "device reads from the peer.";
    }
};
