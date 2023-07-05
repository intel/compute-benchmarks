/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "emu_benchmark";
    const std::string description = "Emulation Benchmark is a set of tests aimed at measuring performance of emulated math operations performed in kernels.";
    const int testCaseColumnWidth = 61;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
