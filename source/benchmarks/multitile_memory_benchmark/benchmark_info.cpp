/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "multitile_memory_benchmark";
    const std::string description = "Multi-tile Memory Benchmark is a set of tests aimed at measuring bandwidth of memory transfers performed on a multi-tile device.";
    const int testCaseColumnWidth = 136;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
