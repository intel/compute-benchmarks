/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/measurement_unit.h"

#include <memory>
#include <optional>
#include <string>

struct ArgumentContainer;

class BenchmarkInfo {
  private:
  public:
    static BenchmarkInfo &get();
    static void initialize(const std::string &name,
                           const std::string &description);
    static void initialize(const std::string &name,
                           const std::string &description,
                           std::optional<size_t> defaultIterations,
                           std::optional<size_t> defaultWarmupIterations);

    std::string getBenchmarkName() const;
    std::string getBenchmarkFilename() const;
    std::string getBenchmarkDescription() const;
    std::optional<size_t> getDefaultIterations() const;
    std::optional<size_t> getDefaultWarmupIterations() const;

  private:
    static std::unique_ptr<BenchmarkInfo> instance;
    std::string name{};
    std::string description{};
    std::optional<size_t> defaultIterations{};
    std::optional<size_t> defaultWarmupIterations{};
};
