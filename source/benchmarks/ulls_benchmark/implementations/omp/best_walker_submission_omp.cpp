/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/linux/set_env_raii.h"
#include "framework/utility/timer.h"

#include "definitions/best_walker_submission.h"

#include <omp.h>

void writeOne(uint32_t *buffer) {
#pragma omp target
    {
        *buffer = 1u;
    }
}

static TestResult run(const BestWalkerSubmissionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    SetEnvRAII("OMP_TARGET_OFFLOAD", "MANDATORY");
    SetEnvRAII("LIBOMPTARGET_LEVEL_ZERO_COMMAND_MODE", "async");
    auto deviceId = omp_get_default_device();
    Timer timer;

    // Create buffer
    auto buffer = static_cast<uint32_t *>(omp_target_alloc_host(sizeof(uint32_t), deviceId));
    volatile auto volatileBuffer = buffer;

    // Warmup
    writeOne(buffer);

    // Benchmark
#pragma omp parallel num_threads(2)
    {
        const auto threadId = omp_get_thread_num();

        for (auto i = 0u; i < arguments.iterations; i++) {
            if (threadId == 0) {
                *buffer = 0u;

                timer.measureStart();
            }
#pragma omp barrier
            if (threadId == 0) {
                while (*volatileBuffer != 1u) {
                }

                timer.measureEnd();

                statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
            } else {
                writeOne(buffer);
            }
#pragma omp barrier
        }
    }

    // Cleanup
    omp_target_free(buffer, deviceId);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerSubmission> registerTestCase(run, Api::OMP);
