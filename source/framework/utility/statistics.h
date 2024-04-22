/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/measurement_type.h"
#include "framework/enum/measurement_unit.h"
#include "framework/utility/error.h"

#include <chrono>

class Statistics {
  public:
    using Clock = std::chrono::high_resolution_clock;

    Statistics(size_t maxSamplesCount) : maxSamplesCount(maxSamplesCount) {}

    virtual void pushPercentage(double value, MeasurementUnit unit, MeasurementType type, const std::string &description = "") = 0;
    virtual void pushValue(Clock::duration time, MeasurementUnit unit, MeasurementType type, const std::string &description = "") = 0;
    virtual void pushValue(Clock::duration time, uint64_t size, MeasurementUnit unit, MeasurementType type, const std::string &description = "") = 0;
    virtual void pushEnergy(size_t microJoules, MeasurementUnit unit, MeasurementType type, const std::string &description = "") = 0;
    virtual void pushEnergy(double watts, MeasurementUnit unit, MeasurementType type, const std::string &description = "") = 0;
    virtual void pushUnitAndType(MeasurementUnit unit, MeasurementType type) = 0;

    virtual bool isEmpty() const = 0;
    virtual bool isFull() const = 0;

  protected:
    const size_t maxSamplesCount = 0;
};

class MeasurementFields {
  public:
    MeasurementFields(MeasurementUnit unit, MeasurementType type) : unit{unit}, type{type} {};

    MeasurementUnit getUnit() const {
        return unit;
    }

    MeasurementType getType() const {
        return type;
    }

  private:
    MeasurementUnit unit{MeasurementUnit::Unknown};
    MeasurementType type{MeasurementType::Unknown};
};
