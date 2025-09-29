/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "decoder2_impl_l0.h"
#include "definitions/decoder2_graph.h"

static TestResult run(const Decoder2GraphArguments &arguments,
                      Statistics &statistics) {
    auto decoder = Decoder2GraphL0(arguments);
    return decoder.run(statistics);
}

[[maybe_unused]] static RegisterTestCaseImplementation<Decoder2Graph>
    registerTestCase(run, Api::L0);
