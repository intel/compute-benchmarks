/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/usm_eu_copy_write_chunked.h"
#include "utility/l0/peer_copy_utility_l0.h"

static TestResult run(const UsmEuCopyWriteChunkedArguments &arguments, Statistics &statistics) {
    return measurePeerCopies(arguments, {PeerCopy{PeerCopySubmitter::Local, PeerCopyDirection::Write, arguments.engine}}, PeerCopyOptions{PeerCopyKernel::Copy, arguments.chunkSize, 0}, statistics);
}

static RegisterTestCaseImplementation<UsmEuCopyWriteChunked> registerTestCase(run, Api::L0);
