/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "cpu_efficiency_benchmark";
    const std::string description = "CPU Efficiency Benchmark measures CPU cost, utilization, and latency tradeoffs of host-side wait and synchronization paths.";
    BenchmarkInfo::initialize(name, description);
};
