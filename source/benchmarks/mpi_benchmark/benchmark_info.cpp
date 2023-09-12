/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_info.h"

#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    const std::string name = "mpi_benchmark";
    const std::string description = "MPI Benchmark is a set of tests aimed at measuring the performance of hybrid MPI+X applications.";
    const int testCaseColumnWidth = 125;
    BenchmarkInfo::initialize(name, description, testCaseColumnWidth);
};
