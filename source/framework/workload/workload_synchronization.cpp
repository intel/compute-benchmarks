/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "workload_synchronization.h"

#include "framework/utility/error.h"
#include "framework/utility/process_synchronization_helper.h"
#include "framework/workload/workload_io.h"

WorkloadSynchronization::WorkloadSynchronization(size_t iterationsCount, bool synchronizationEnabled)
    : expectedSynchronizationCount(iterationsCount),
      synchronizationEnabled(synchronizationEnabled) {
}

void WorkloadSynchronization::synchronize(WorkloadIo &workloadIo) {
    FATAL_ERROR_IF(synchronizationCount == expectedSynchronizationCount, "Too many iterations signalled during workload synchronization");
    synchronizationCount++;

    if (!synchronizationEnabled) {
        return;
    }

    // Signal that we're ready
    workloadIo.writeSynchronizationChar(ProcessSynchronizationHelper::synchronizationChar);

    // Wait for signal from master
    char character = {};
    do {
        character = workloadIo.readSynchronizationChar();
    } while (character == '\n' || character == '\r');

    FATAL_ERROR_IF(character != ProcessSynchronizationHelper::synchronizationChar, std::string("Invalid synchronization received from parent process: '") + character + "'");
}

bool WorkloadSynchronization::validate() {
    if (synchronizationEnabled) {
        return synchronizationCount == expectedSynchronizationCount;
    } else {
        return true;
    }
}

void WorkloadSynchronization::executeRemainingSynchronizations(WorkloadIo &workloadIo) {
    while (synchronizationCount < expectedSynchronizationCount) {
        synchronize(workloadIo);
    }
}
