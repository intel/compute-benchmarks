/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "page_fault_benchmark";
    const std::string description = "Page Fault Benchmark is a set of tests aimed at measuring bandwidth of memory migration.";
    const int testCaseColumnWidth = 126;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
