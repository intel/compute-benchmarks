/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "decoder2_impl_sycl.h"
#include "definitions/decoder2_graph.h"

static TestResult run(const Decoder2GraphArguments &arguments,
                      Statistics &statistics) {
    auto decoder = Decoder2GraphSYCL(arguments);
    return decoder.run(statistics);
}

[[maybe_unused]] static RegisterTestCaseImplementation<Decoder2Graph>
    registerTestCase(run, Api::SYCL);
