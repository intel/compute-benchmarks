/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/statistics.h"

#include <chrono>
#include <sstream>

class WorkloadIo;

class WorkloadStatistics : public Statistics {
  public:
    using Statistics::Statistics;
    using Clock = std::chrono::high_resolution_clock;

    void printStatistics(WorkloadIo &io);

    void pushPercentage(double value, MeasurementUnit unit, MeasurementType type, const std::string &description = "") override;
    void pushValue(Clock::duration time, MeasurementUnit unit, MeasurementType type, const std::string &description = "") override;
    void pushValue(Clock::duration time, uint64_t size, MeasurementUnit unit, MeasurementType type, const std::string &description = "") override;
    void pushUnitAndType(MeasurementUnit unit, MeasurementType type) override;

    bool isEmpty() const override;
    bool isFull() const override;

  private:
    std::ostringstream result{};
    size_t samplesCount = 0;
};
