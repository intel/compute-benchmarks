/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/linux/error.h"
#include "framework/utility/process_synchronization_helper.h"
#include "framework/workload/workload_io.h"

#include <iostream>
#include <unistd.h>

class WorkloadIoLinux : public WorkloadIo {
  public:
    WorkloadIoLinux(int synchronizationPipeIn, int synchronizationPipeOut, int measurementPipe)
        : synchronizationPipeIn(synchronizationPipeIn),
          synchronizationPipeOut(synchronizationPipeOut),
          measurementPipe(measurementPipe) {}

    void writeToConsole(const std::string &message) override {
        std::cerr << message;
    }

    void writeToMeasurements(const std::string &measurements) override {
        if (measurementPipe) {
            ssize_t numberOfBytesWritten = write(measurementPipe, measurements.c_str(), measurements.size());
            FATAL_ERROR_IF_SYS_CALL_FAILED(numberOfBytesWritten, "Writing measurements in a child process failed");
            FATAL_ERROR_IF(numberOfBytesWritten == 0, "No character was written when writing measurements in a child process");
        } else {
            std::cout << measurements << ' ';
        }
    }

    void writeSynchronizationChar(char c) override {
        if (synchronizationPipeOut) {
            char buffer = ProcessSynchronizationHelper::synchronizationChar;
            ssize_t numberOfBytesWritten = write(synchronizationPipeOut, &buffer, 1);
            FATAL_ERROR_IF_SYS_CALL_FAILED(numberOfBytesWritten, "Writing synchronization char in a child process failed");
            FATAL_ERROR_IF(numberOfBytesWritten == 0, "No character was written when writing synchronization char in a child process");
        } else {
            std::cout << c;
        }
    }

    char readSynchronizationChar() override {
        if (synchronizationPipeIn) {
            char buffer = {};
            ssize_t numberOfBytesRead = read(synchronizationPipeIn, &buffer, 1u);
            FATAL_ERROR_IF_SYS_CALL_FAILED(numberOfBytesRead, "Reading synchronization char in a child process failed");
            FATAL_ERROR_IF(numberOfBytesRead == 0, "No character was read when reading synchronization char in a child process");
            FATAL_ERROR_IF(buffer != ProcessSynchronizationHelper::synchronizationChar, "Invalid synchronization received in a child process from the parent: '", buffer, "'");
            return buffer;
        } else {
            return std::cin.get();
        }
    }

  private:
    const int synchronizationPipeIn;
    const int synchronizationPipeOut;
    const int measurementPipe;
};

std::unique_ptr<WorkloadIo> WorkloadIo::create(const WorkloadArgumentContainer &arguments) {
    return std::unique_ptr<WorkloadIo>(new WorkloadIoLinux(arguments.synchronizationPipeIn, arguments.synchronizationPipeOut, arguments.measurementPipe));
}
