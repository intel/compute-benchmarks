/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/measurement_type.h"
#include "framework/enum/measurement_unit.h"
#include "framework/utility/process.h"

#include <string>

class Statistics;

class ProcessGroup {
  public:
    ProcessGroup(const std::string &binaryName, size_t count);

    void addArgumentAll(const std::string &key, const std::string &value);
    void addEnvVariableAll(const std::string &key, const std::string &value);
    void runAll();
    void synchronizeAll(size_t iterationsCount);
    void waitForFinishAll();
    TestResult getResultAll();

    void pushMeasurementsToStatistics(size_t expectedCount,
                                      Statistics &statistics,
                                      MeasurementUnit unit,
                                      MeasurementType type,
                                      bool pushIndividualProcessesMeasurements,
                                      bool pushAveragedMeasurements);

    Process &operator[](size_t index);
    size_t size() const;

  private:
    const std::string binaryName;
    std::vector<Process> processes = {};
};
