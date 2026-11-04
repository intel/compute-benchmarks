/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_copy_write_read.h"
#include "utility/l0/peer_copy_utility_l0.h"

static TestResult run(const UsmCopyWriteReadArguments &arguments, Statistics &statistics) {
    return measurePeerCopies(arguments, {PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Write, arguments.writeEngine}, PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Read, arguments.readEngine}}, statistics);
}

static RegisterTestCaseImplementation<UsmCopyWriteRead> registerTestCase(run, Api::L0);
