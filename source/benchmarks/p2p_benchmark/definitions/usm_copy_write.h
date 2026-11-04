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

struct UsmCopyWriteArguments : PeerCopyArguments {
    EngineArgument engine;

    UsmCopyWriteArguments()
        : engine(*this, "engine", "Engine of the local device, which writes to the peer") {}
};

struct UsmCopyWrite : TestCase<UsmCopyWriteArguments> {
    using TestCase<UsmCopyWriteArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyWrite";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of a copy from the "
               "local device to the peer device, submitted by the local device on an immediate command list, so the local "
               "device writes to the peer.";
    }
};
