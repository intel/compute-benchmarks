/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "gpu_cmds_benchmark";
    const std::string description = "Gpu Commands Benchmark is a set of tests aimed at measuring GPU-side execution duration of various commands.";
    const int testCaseColumnWidth = 120;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
