/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "gpu_cmds_benchmark";
    const std::string description = "Gpu Commands Benchmark is a set of tests aimed at measuring GPU-side execution duration of various commands.";
    BenchmarkInfo::initialize(name, description);
};
