/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_push_pull.h"
#include "utility/l0/peer_copy_utility_l0.h"

static TestResult run(const UsmCopyPushPullArguments &arguments, Statistics &statistics) {
    return measurePeerCopies(arguments, {PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Write, arguments.writeEngine}, PeerCopy{PeerCopySubmitter::Peer, PeerCopyDirection::Read, arguments.readEngine}}, statistics);
}

static RegisterTestCaseImplementation<UsmCopyPushPull> registerTestCase(run, Api::L0);
