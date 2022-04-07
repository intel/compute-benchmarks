/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "process_group.h"

#include "framework/utility/error.h"
#include "framework/utility/statistics.h"
#include "framework/utility/string_utils.h"

ProcessGroup::ProcessGroup(const std::string &binaryName, size_t count)
    : binaryName(binaryName) {
    for (auto processIndex = 0u; processIndex < count; processIndex++) {
        processes.emplace_back(binaryName);
    }
}

void ProcessGroup::addArgumentAll(const std::string &key, const std::string &value) {
    for (Process &process : processes) {
        process.addArgument(key, value);
    }
}

void ProcessGroup::addEnvVariableAll(const std::string &key, const std::string &value) {
    for (Process &process : processes) {
        process.addEnvVariable(key, value);
    }
}

void ProcessGroup::runAll() {
    for (Process &process : processes) {
        process.run();
    }
}

void ProcessGroup::synchronizeAll(size_t iterationsCount) {
    for (auto iteration = 0u; iteration < iterationsCount; iteration++) {
        for (Process &process : processes) {
            process.synchronizationWait();
        }

        for (Process &process : processes) {
            process.synchronizationSignal();
        }
    }
}

void ProcessGroup::waitForFinishAll() {
    for (Process &process : processes) {
        process.waitForFinish();
    }
}

TestResult ProcessGroup::getResultAll() {
    for (Process &process : processes) {
        const auto result = process.getResult();
        if (result != TestResult::Success) {
            return result;
        }
    }
    return TestResult::Success;
}

void ProcessGroup::pushMeasurementsToStatistics(size_t expectedCount,
                                                Statistics &statistics,
                                                MeasurementUnit unit,
                                                MeasurementType type,
                                                bool pushIndividualProcessesMeasurements,
                                                bool pushAveragedMeasurements) {
    std::vector<uint64_t> averagedMeasurements(expectedCount);

    for (Process &process : processes) {
        const auto measurementsFromProcesses = process.getMeasurements(expectedCount);

        for (auto measurementIndex = 0u; measurementIndex < measurementsFromProcesses.size(); measurementIndex++) {
            const auto &measurement = measurementsFromProcesses[measurementIndex];

            if (pushIndividualProcessesMeasurements) {
                statistics.pushValue(std::chrono::nanoseconds(measurement), unit, type, process.getName());
            }

            if (pushAveragedMeasurements) {
                averagedMeasurements[measurementIndex] += measurement;
            }
        }
    }

    if (pushAveragedMeasurements) {
        for (auto &averagedMeasurement : averagedMeasurements) {
            averagedMeasurement /= processes.size();
            statistics.pushValue(std::chrono::nanoseconds(averagedMeasurement), unit, type);
        }
    }
}

Process &ProcessGroup::operator[](size_t index) {
    FATAL_ERROR_IF(index >= processes.size(), "Invalid process index");
    return processes[index];
}

size_t ProcessGroup::size() const {
    return processes.size();
}
