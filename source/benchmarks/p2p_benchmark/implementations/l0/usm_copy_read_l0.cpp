/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_read.h"
#include "utility/l0/peer_copy_utility_l0.h"

static TestResult run(const UsmCopyReadArguments &arguments, Statistics &statistics) {
    return measurePeerCopies(arguments, {PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Read, arguments.engine}}, statistics);
}

static RegisterTestCaseImplementation<UsmCopyRead> registerTestCase(run, Api::L0);
