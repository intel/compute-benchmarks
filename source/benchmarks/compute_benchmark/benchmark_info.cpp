/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "compute_benchmark";
    const std::string description = "Compute Benchmark is a set of tests aimed at measuring floating point compute throughput of GPU kernels.";
    BenchmarkInfo::initialize(name, description);
};
