/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_concurrent_pairs_read.h"
#include "utility/l0/peer_copy_pairs_utility_l0.h"

static TestResult run(const UsmCopyConcurrentPairsReadArguments &arguments, Statistics &statistics) {
    return measurePeerCopyPairs(arguments, PeerCopyDirection::Read, statistics);
}

static RegisterTestCaseImplementation<UsmCopyConcurrentPairsRead> registerTestCase(run, Api::L0);
