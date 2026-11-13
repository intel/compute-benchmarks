/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "benchmarks/memory_benchmark/blit_size_assigner.h"
#include "utility/l0/peer_copy_utility_l0.h"

#include <algorithm>

inline TestResult addPeerCopyBlitterSegments(std::vector<PeerCopy> &copies, PeerCopySubmitter submitter, PeerCopyDirection direction,
                                             const PeerCopyBlittersArgument &blitters, size_t size) {
    const std::vector<size_t> blitterIndices = blitters.getEnabledBits();
    BlitSizeAssigner blitSizeAssigner{size};
    for (const size_t blitterIndex : blitterIndices) {
        if (blitterIndex == 0) {
            blitSizeAssigner.addMainCopyEngine();
        } else {
            blitSizeAssigner.addLinkCopyEngine();
        }
    }
    for (const size_t blitterIndex : blitterIndices) {
        PeerCopy copy{submitter, direction, EngineHelper::getBlitterEngineFromIndex(blitterIndex)};
        copy.group = copy.getLabel();
        copy.label = copy.group + EngineHelper::getEngineName(copy.engine);
        copy.size = blitSizeAssigner.getSpaceForBlit(blitterIndex == 0).second;
        if (copy.size == 0) {
            return TestResult::InvalidArgs;
        }
        copies.push_back(copy);
    }
    return TestResult::Success;
}

inline TestResult measurePeerCopyBlitterSegments(const PeerCopyArguments &arguments,
                                                 const std::vector<std::tuple<PeerCopySubmitter, PeerCopyDirection, const PeerCopyBlittersArgument *>> &segmentedCopies,
                                                 Statistics &statistics) {
    const bool anyCopySplit = std::any_of(segmentedCopies.begin(), segmentedCopies.end(), [](const auto &segmentedCopy) {
        return std::get<2>(segmentedCopy)->getEnabledBits().size() > 1;
    });
    if (!anyCopySplit) {
        return TestResult::InvalidArgs;
    }
    std::vector<PeerCopy> copies;
    for (const auto &[submitter, direction, blitters] : segmentedCopies) {
        ASSERT_TEST_RESULT_SUCCESS(addPeerCopyBlitterSegments(copies, submitter, direction, *blitters, arguments.size));
    }
    return measurePeerCopies(arguments, std::move(copies), statistics);
}
