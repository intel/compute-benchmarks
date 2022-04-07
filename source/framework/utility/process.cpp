/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "process.h"

#include "framework/utility/error.h"
#include "framework/utility/string_utils.h"

#include <iostream>

Process::Process(const std::string &exeName)
    : exeName(exeName) {
    arguments.emplace_back(exeName, std::string{});
}

Process::Process(Process &&other)
    : exeName(std::move(other.exeName)),
      arguments(std::move(other.arguments)),
      envVariables(std::move(other.envVariables)),
      osSpecificData(std::move(other.osSpecificData)) {
    other.osSpecificData = nullptr;
}

Process &Process::operator=(Process &&other) {
    exeName = std::move(other.exeName);
    arguments = std::move(other.arguments);
    envVariables = std::move(other.envVariables);
    osSpecificData = std::move(other.osSpecificData);
    other.osSpecificData = nullptr;
    return *this;
}

Process::~Process() {
    freeOsSpecificData();
}

void Process::addArgument(const std::string &key, const std::string &value) {
    arguments.emplace_back(std::string("--") + key, value);
}

void Process::addEnvVariable(const std::string &key, const std::string &value) {
    envVariables.emplace_back(key, value);
}

void Process::addHandleForInheritance(int handle) {
    handlesForInheritance.push_back(handle);
}

std::vector<uint64_t> Process::getMeasurements(size_t expectedCount) {
    const auto data = getMeasurements();
    const auto dataSplit = splitString(data);
    FATAL_ERROR_IF(dataSplit.size() != expectedCount, "Child process returned an invalid number of measurements");

    std::vector<uint64_t> measurementsFromProcess = {};
    for (const auto &measurementString : dataSplit) {
        const auto measurement = std::atoll(measurementString.c_str());
        measurementsFromProcess.push_back(measurement);
    }

    return measurementsFromProcess;
}
