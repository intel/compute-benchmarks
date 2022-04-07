/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/benchmark_main.h"

int main(int argc, char **argv) {
    BenchmarkMain benchmarkMain{argc, argv, BENCHMARK_VERSION};
    return benchmarkMain.main();
}
