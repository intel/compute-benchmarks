/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstddef>

class WorkloadIo;

class WorkloadSynchronization {
  public:
    WorkloadSynchronization(size_t iterationsCount, bool synchronizationEnabled);

    void synchronize(WorkloadIo &workloadIo);
    bool validate();

    void executeRemainingSynchronizations(WorkloadIo &workloadIo);

  private:
    size_t synchronizationCount = 0;
    size_t expectedSynchronizationCount;
    bool synchronizationEnabled;
};
