/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "multithread_benchmark";
    const std::string description = "Multithread Benchmark is a set of tests aimed at measuring how different commands benefit from multithreaded execution.";
    const int testCaseColumnWidth = 135;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
