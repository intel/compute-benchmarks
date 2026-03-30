/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "record_and_replay_benchmark";
    const std::string description = "Record and Replay Benchmark is a set of tests aimed at measuring performance of recording and execution of commandlist graphs.";
    BenchmarkInfo::initialize(name, description);
};
