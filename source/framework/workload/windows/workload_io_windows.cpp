/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/error.h"
#include "framework/utility/process_synchronization_helper.h"
#include "framework/utility/windows/windows.h"
#include "framework/workload/workload_io.h"

#include <cstdint>
#include <iostream>

class WorkloadIoWindows : public WorkloadIo {
  public:
    WorkloadIoWindows(HANDLE synchronizationPipeIn, HANDLE synchronizationPipeOut, HANDLE measurementPipe)
        : synchronizationPipeIn(synchronizationPipeIn),
          synchronizationPipeOut(synchronizationPipeOut),
          measurementPipe(measurementPipe) {}

    void writeToConsole(const std::string &message) override {
        std::cerr << message;
    }

    void writeToMeasurements(const std::string &measurements) override {
        if (measurementPipe) {
            const std::string toWrite = measurements + ' ';
            DWORD numberOfBytesWritten = {};
            FATAL_ERROR_IF_SYS_CALL_FAILED(WriteFile(measurementPipe, toWrite.c_str(), static_cast<DWORD>(toWrite.size()), &numberOfBytesWritten, nullptr), "Writing measurements in a child process failed");
            FATAL_ERROR_IF(static_cast<size_t>(numberOfBytesWritten) != toWrite.size(), "Incomplete write when writing measurements in a child process");
        } else {
            std::cout << measurements << ' ';
        }
    }

    void writeSynchronizationChar(char c) override {
        if (synchronizationPipeOut) {
            char buffer = ProcessSynchronizationHelper::synchronizationChar;
            DWORD numberOfBytesWritten = {};
            FATAL_ERROR_IF_SYS_CALL_FAILED(WriteFile(synchronizationPipeOut, &buffer, 1, &numberOfBytesWritten, nullptr), "Writing synchronization char in a child process failed");
            FATAL_ERROR_IF(numberOfBytesWritten == 0, "No character was written when writing synchronization char in a child process");
        } else {
            std::cout << c;
        }
    }

    char readSynchronizationChar() override {
        if (synchronizationPipeIn) {
            char buffer = {};
            DWORD numberOfBytesRead = {};
            FATAL_ERROR_IF_SYS_CALL_FAILED(ReadFile(synchronizationPipeIn, &buffer, 1, &numberOfBytesRead, NULL), "Reading synchronization char in a child process failed");
            FATAL_ERROR_IF(numberOfBytesRead == 0, "No character was read when reading synchronization char in a child process");
            FATAL_ERROR_IF(buffer != ProcessSynchronizationHelper::synchronizationChar, std::string("Invalid synchronization received in a child process from the parent: '") + buffer + "'");
            return buffer;
        } else {
            return static_cast<char>(std::cin.get());
        }
    }

  private:
    const HANDLE synchronizationPipeIn;
    const HANDLE synchronizationPipeOut;
    const HANDLE measurementPipe;
};

static HANDLE argumentToHandle(int64_t value) {
    return reinterpret_cast<HANDLE>(static_cast<uintptr_t>(value));
}

std::unique_ptr<WorkloadIo> WorkloadIo::create(const WorkloadArgumentContainer &arguments) {
    return std::unique_ptr<WorkloadIo>(new WorkloadIoWindows(argumentToHandle(arguments.synchronizationPipeIn),
                                                             argumentToHandle(arguments.synchronizationPipeOut),
                                                             argumentToHandle(arguments.measurementPipe)));
}
