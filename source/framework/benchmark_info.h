/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/measurement_unit.h"

#include <memory>
#include <string>

struct ArgumentContainer;

// This class represent information specific to a specific benchmark, e.g. ulls_benchmark or memory_benchmark.
// It is used to configure the behaviour of some framework classes.
class BenchmarkInfo {
  private:
  public:
    // Singleton accessors
    static BenchmarkInfo &get();
    static void initialize(const std::string &name,
                           const std::string &description);

    // Getters
    std::string getBenchmarkName() const;
    std::string getBenchmarkFilename() const;
    std::string getBenchmarkDescription() const;

  private:
    static std::unique_ptr<BenchmarkInfo> instance;
    std::string name{};
    std::string description{};
};
