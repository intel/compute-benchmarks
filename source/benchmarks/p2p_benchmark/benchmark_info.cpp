/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "p2p_benchmark";
    const std::string description = "P2P Benchmark is a set of tests aimed at measuring bandwidth and latency of memory transfers between peer devices.";
    const int testCaseColumnWidth = 126;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
