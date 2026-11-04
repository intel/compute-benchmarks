/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_bidirectional_write.h"
#include "utility/l0/peer_copy_utility_l0.h"

static TestResult run(const UsmCopyBidirectionalWriteArguments &arguments, Statistics &statistics) {
    return measurePeerCopies(arguments, {PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Write, arguments.localEngine}, PeerCopy{PeerCopySubmitter::Peer, PeerCopyDirection::Write, arguments.peerEngine}}, statistics);
}

static RegisterTestCaseImplementation<UsmCopyBidirectionalWrite> registerTestCase(run, Api::L0);
