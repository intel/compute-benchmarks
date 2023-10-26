/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "api_overhead_benchmark";
    const std::string description = "Api Overhead Benchmark is a set of tests aimed at measuring CPU-side execution duration of compute API calls.";
    const int testCaseColumnWidth = 130;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
