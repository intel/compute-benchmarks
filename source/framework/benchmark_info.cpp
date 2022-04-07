/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "benchmark_info.h"

#include "framework/utility/error.h"

std::unique_ptr<BenchmarkInfo> BenchmarkInfo::instance = {};

BenchmarkInfo &BenchmarkInfo::get() {
    FATAL_ERROR_IF(instance == nullptr, "BenchmarkInfo::get() called before BenchmarkInfo::initialize()");
    return *instance;
}

void BenchmarkInfo::initialize(const std::string &name, const std::string &description, int testCaseColumnWidth) {
    FATAL_ERROR_IF(instance != nullptr, "BenchmarkInfo::initialize() called multiple times");

    instance = std::make_unique<BenchmarkInfo>();
    instance->name = name;
    instance->description = description;
    instance->testCaseColumnWidth = testCaseColumnWidth;
}

std::string BenchmarkInfo::getBenchmarkName() const {
    return name;
}

std::string BenchmarkInfo::getBenchmarkFilename() const {
#ifdef WIN32
    return getBenchmarkName() + ".exe";
#else
    return getBenchmarkName();
#endif
}

std::string BenchmarkInfo::getBenchmarkDescription() const {
    return description;
}

int BenchmarkInfo::getTestCaseNameColumnWidth() const {
    return testCaseColumnWidth;
}
