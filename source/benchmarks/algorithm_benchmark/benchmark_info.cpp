/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "algorithm_benchmark";
    const std::string description = "Algorithm Benchmark is a set of benchmarks aimed at measuring the performance of realistic worloads.";
    BenchmarkInfo::initialize(name, description);
};
