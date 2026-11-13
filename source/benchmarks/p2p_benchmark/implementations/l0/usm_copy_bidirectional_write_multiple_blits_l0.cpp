/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_bidirectional_write_multiple_blits.h"
#include "utility/l0/peer_copy_multiple_blits_utility_l0.h"

static TestResult run(const UsmCopyBidirectionalWriteMultipleBlitsArguments &arguments, Statistics &statistics) {
    return measurePeerCopyBlitterSegments(arguments, {{PeerCopySubmitter::Local, PeerCopyDirection::Write, &arguments.localBlitters}, {PeerCopySubmitter::Peer, PeerCopyDirection::Write, &arguments.peerBlitters}}, statistics);
}

static RegisterTestCaseImplementation<UsmCopyBidirectionalWriteMultipleBlits> registerTestCase(run, Api::L0);
