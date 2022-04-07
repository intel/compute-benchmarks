/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/workload/workload_io.h"

#include <iostream>

class WorkloadIoWindows : public WorkloadIo {
  public:
    void writeToConsole(const std::string &message) override {
        std::cerr << message;
    }

    void writeToMeasurements(const std::string &measurements) override {
        std::cout << measurements << ' ';
    }

    void writeSynchronizationChar(char c) override {
        std::cout << c;
    }

    char readSynchronizationChar() override {
        return static_cast<char>(std::cin.get());
    }
};

std::unique_ptr<WorkloadIo> WorkloadIo::create([[maybe_unused]] const WorkloadArgumentContainer &arguments) {
    return std::unique_ptr<WorkloadIo>(new WorkloadIoWindows());
}
