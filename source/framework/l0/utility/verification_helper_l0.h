/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/test_case/test_result.h"

#include <cstddef>
#include <cstdint>

class VerificationHelperL0 {
  public:
    explicit VerificationHelperL0(L0::LevelZero &levelzero) : levelzero(levelzero) {}
    VerificationHelperL0(const VerificationHelperL0 &) = delete;
    VerificationHelperL0 &operator=(const VerificationHelperL0 &) = delete;
    ~VerificationHelperL0();

    ze_result_t initialize(size_t outputSize);
    ze_result_t poison(void *buffer, size_t size);
    const uint8_t *read(const void *buffer, size_t offset, size_t size);

    TestResult initializeSampling(const char *programName, const char *buildOptions, void *output, void *samples,
                                  size_t sampleCount, size_t elementCount, size_t groupSize, size_t multiplier);
    ze_result_t resetSamples();
    ze_result_t gather();

  private:
    ze_result_t execute();

    L0::LevelZero &levelzero;
    ze_command_list_handle_t cmdList{};
    void *staging{};
    ze_result_t launch(ze_kernel_handle_t kernel);

    ze_module_handle_t samplingModule{};
    ze_kernel_handle_t gatherKernel{};
    ze_kernel_handle_t resetKernel{};
    ze_group_count_t samplingGroupCount{};
};
