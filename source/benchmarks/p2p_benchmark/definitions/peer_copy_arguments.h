/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/enum/engine.h"
#include "framework/test_case/test_case_argument_container.h"
#include "framework/utility/common_help_message.h"

#include <limits>

struct PeerCopyArguments : TestCaseArgumentContainer {
    static constexpr size_t reduceCopyElementSize = 16u;
    static constexpr uint32_t reduceCopyWorkGroupSize = 64u;
    static constexpr size_t statelessBufferSize = 4ull * 1024 * 1024 * 1024;

    NonNegativeIntegerArgument localDeviceId;
    NonNegativeIntegerArgument peerDeviceId;
    ByteSizeArgument size;
    BooleanArgument useEvents;

    PeerCopyArguments()
        : localDeviceId(*this, "localDeviceId", "Local root device"),
          peerDeviceId(*this, "peerDeviceId", "Peer root device"),
          size(*this, "size", "Size of each copy"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        return localDeviceId != peerDeviceId;
    }

    static bool isComputeEngine(Engine engine) {
        const EngineGroup group = EngineHelper::getEngineGroup(engine, EngineHelper::maxNumberOfCopyEngines);
        return group == EngineGroup::Compute || group == EngineGroup::RenderCompute;
    }

    bool isValidReduceCopySize(size_t chunkSize) const {
        return size % reduceCopyElementSize == 0 &&
               chunkSize % reduceCopyElementSize == 0 &&
               chunkSize / reduceCopyElementSize / reduceCopyWorkGroupSize < std::numeric_limits<uint32_t>::max();
    }

    bool isValidThrottle(size_t throttledWorkItems) const {
        return throttledWorkItems % reduceCopyWorkGroupSize == 0 &&
               throttledWorkItems / reduceCopyWorkGroupSize <= std::numeric_limits<uint32_t>::max() &&
               size % reduceCopyElementSize == 0;
    }
};
