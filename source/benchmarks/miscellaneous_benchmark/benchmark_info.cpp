/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "miscellaneous_benchmark";
    const std::string description = "Miscellaneous Benchmark is a set of tests measuring different simple compute scenarios.";
    const int testCaseColumnWidth = 90;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
