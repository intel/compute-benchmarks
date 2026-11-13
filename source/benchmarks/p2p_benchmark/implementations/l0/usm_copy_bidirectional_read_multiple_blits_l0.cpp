/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_bidirectional_read_multiple_blits.h"
#include "utility/l0/peer_copy_multiple_blits_utility_l0.h"

static TestResult run(const UsmCopyBidirectionalReadMultipleBlitsArguments &arguments, Statistics &statistics) {
    return measurePeerCopyBlitterSegments(arguments, {{PeerCopySubmitter::Local, PeerCopyDirection::Read, &arguments.localBlitters}, {PeerCopySubmitter::Peer, PeerCopyDirection::Read, &arguments.peerBlitters}}, statistics);
}

static RegisterTestCaseImplementation<UsmCopyBidirectionalReadMultipleBlits> registerTestCase(run, Api::L0);
