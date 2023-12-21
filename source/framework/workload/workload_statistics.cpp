/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "workload_statistics.h"

#include "framework/utility/error.h"
#include "framework/workload/workload_io.h"

#include <iostream>

void WorkloadStatistics::pushPercentage(double value, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    FATAL_ERROR_IF(type != MeasurementType::Unknown, "WorkloadStatistics does not support setting measurement type");
    FATAL_ERROR_IF(unit != MeasurementUnit::Unknown, "WorkloadStatistics does not support setting measurement type");
    FATAL_ERROR_IF(description != "", "WorkloadStatistics does not support multiple statistics groups");
    FATAL_ERROR_IF(samplesCount == maxSamplesCount, "Too many values pushed by the test");
    samplesCount++;

    result << value << ' ';
}

void WorkloadStatistics::pushValue(Clock::duration time, MeasurementUnit unit, MeasurementType type, const std::string &description) {
    FATAL_ERROR_IF(type != MeasurementType::Unknown, "WorkloadStatistics does not support setting measurement type");
    FATAL_ERROR_IF(unit != MeasurementUnit::Unknown, "WorkloadStatistics does not support setting measurement type");
    FATAL_ERROR_IF(description != "", "WorkloadStatistics does not support multiple statistics groups");
    FATAL_ERROR_IF(samplesCount == maxSamplesCount, "Too many values pushed by the test");
    samplesCount++;

    const auto timeNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(time).count();
    result << timeNanoseconds << ' ';
}

void WorkloadStatistics::pushValue([[maybe_unused]] Clock::duration time,
                                   [[maybe_unused]] uint64_t size,
                                   [[maybe_unused]] MeasurementUnit unit,
                                   MeasurementType type,
                                   const std::string &description) {
    FATAL_ERROR_IF(type != MeasurementType::Unknown, "WorkloadStatistics does not support setting measurement type");
    FATAL_ERROR_IF(description != "", "WorkloadStatistics does not support multiple statistics groups");
    FATAL_ERROR("Not implemented");
}

void WorkloadStatistics::pushUnitAndType([[maybe_unused]] MeasurementUnit unit, [[maybe_unused]] MeasurementType type) {}

bool WorkloadStatistics::isEmpty() const {
    return samplesCount == 0;
}

bool WorkloadStatistics::isFull() const {
    return samplesCount == maxSamplesCount;
}

void WorkloadStatistics::printStatistics(WorkloadIo &io) {
    io.writeToMeasurements(result.str());
}
