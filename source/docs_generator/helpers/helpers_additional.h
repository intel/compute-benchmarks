/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "docs_generator/data_types.h"

#include <string>

std::vector<Api> getApisForBenchmark(const Benchmark &benchmark, const std::string &currentLocation) {
    return {Api::L0, Api::OpenCL};
}
