/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "definitions/peer_copy_pairs_arguments.h"
#include "utility/l0/peer_copy_utility_l0.h"

#include <map>

inline std::string getPeerCopyPairLabel(PeerCopyDirection direction, size_t submittingDeviceId, size_t remoteDeviceId) {
    const std::string submitter = std::to_string(submittingDeviceId);
    const std::string remote = std::to_string(remoteDeviceId);
    return direction == PeerCopyDirection::Write ? "Write" + submitter + "To" + remote : "Read" + submitter + "From" + remote;
}

inline TestResult measurePeerCopyPairs(const PeerCopyPairsArguments &arguments, PeerCopyDirection direction, Statistics &statistics) {
    const std::vector<Engine> engines = arguments.getEngines();
    std::map<size_t, size_t> copiesOfDevice;
    std::vector<PeerCopy> copies;
    for (const auto &[localDeviceId, peerDeviceId] : arguments.getPairs()) {
        PeerCopy localCopy{PeerCopySubmitter::Local, direction, engines[copiesOfDevice[localDeviceId]++]};
        localCopy.localDeviceId = localDeviceId;
        localCopy.peerDeviceId = peerDeviceId;
        localCopy.label = getPeerCopyPairLabel(direction, localDeviceId, peerDeviceId);
        copies.push_back(localCopy);
    }
    constexpr const char *totalLabelSortedBeforeCopyLabels = "AllPairs";
    PeerCopyOptions options{};
    options.totalLabel = totalLabelSortedBeforeCopyLabels;
    return measurePeerCopiesBetweenDevices(arguments, arguments.size, arguments.useEvents, std::move(copies), options, statistics);
}
