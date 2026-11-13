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

struct UsmCopyReadMultipleBlitsArguments : PeerCopyArguments {
    PeerCopyBlittersArgument blitters;

    UsmCopyReadMultipleBlitsArguments()
        : blitters(*this, "blitters", "Bit mask of the copy engines of the local device that read from the peer. The rightmost bit is BCS0") {}
};

struct UsmCopyReadMultipleBlits : TestCase<UsmCopyReadMultipleBlitsArguments> {
    using TestCase<UsmCopyReadMultipleBlitsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyReadMultipleBlits";
    }

    std::optional<size_t> getDefaultIterations() const override {
        return 10u;
    }

    std::string getHelp() const override {
        return "allocates unified device memory buffers on two root devices and measures the bandwidth of a copy from the peer device to the local device, submitted by the local device on one immediate command list per selected copy engine, so the local device reads from the peer. The engines are released together by one host-signalled event. The copy is split across the selected copy engines: with the main copy engine (the rightmost bit) and link copy engines the main copy engine copies half of the size and the link copy engines share the other half, otherwise the engines share the size equally. At least two engines must be selected. Reports each engine, and the copy as the time from the first engine start to the last engine end.";
    }
};
