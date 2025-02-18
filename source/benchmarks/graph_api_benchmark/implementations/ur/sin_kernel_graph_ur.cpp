/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/ur.h"

#include "definitions/sin_kernel_graph.h"
#include "sin_kernel_impl_ur.h"

static TestResult run(const SinKernelGraphArguments &arguments,
                      Statistics &statistics) {
    auto sin = SinKernelGraphUR(arguments);
    return sin.run(statistics);
}

[[maybe_unused]] static RegisterTestCaseImplementation<SinKernelGraph>
    registerTestCase(run, Api::UR);
