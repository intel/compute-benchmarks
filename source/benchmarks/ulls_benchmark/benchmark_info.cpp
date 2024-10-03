/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "ulls_benchmark";
    const std::string description = "Ulls Benchmark is a set of tests aimed at measuring Ultra Low Latency Submission (ULLS) performance impact.";
    const int testCaseColumnWidth = 91;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
