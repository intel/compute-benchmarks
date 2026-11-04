/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_eu_copy_bidirectional_write_chunked.h"
#include "utility/l0/peer_copy_utility_l0.h"

static TestResult run(const UsmEuCopyBidirectionalWriteChunkedArguments &arguments, Statistics &statistics) {
    return measurePeerCopies(arguments, {PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Write, arguments.localEngine}, PeerCopy{PeerCopySubmitter::Peer, PeerCopyDirection::Write, arguments.peerEngine}}, PeerCopyOptions{PeerCopyKernel::Copy, arguments.chunkSize, 0}, statistics);
}

static RegisterTestCaseImplementation<UsmEuCopyBidirectionalWriteChunked> registerTestCase(run, Api::L0);
