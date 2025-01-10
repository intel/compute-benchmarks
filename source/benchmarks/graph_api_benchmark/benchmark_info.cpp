/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "graph_api_benchmark";
    const std::string description = "Graph Api Overhead Benchmark is a set of tests aimed at measuring CPU-side execution duration of SYCL Graphs API calls.";
    const int testCaseColumnWidth = 64;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
