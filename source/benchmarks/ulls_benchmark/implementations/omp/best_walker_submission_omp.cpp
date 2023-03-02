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

void writeOne(volatile uint64_t *buffer, Timer *timer) {
#pragma omp task
    {
        if (timer) {
            timer->measureStart();
        }

#pragma omp target teams distribute parallel for
        {
            for (int i = 0; i < 1; ++i) {
                *buffer = 1u;
            }
        }
    }

    while (*buffer != 1u) {
    }

    if (timer) {
        timer->measureEnd();
    }

#pragma omp taskwait
}

static TestResult run(const BestWalkerSubmissionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    SetEnvRAII("OMP_TARGET_OFFLOAD", "MANDATORY");
    auto deviceId = omp_get_default_device();
    Timer timer;

    // Create buffer
    volatile auto buffer = static_cast<uint64_t *>(omp_target_alloc_host(sizeof(uint64_t), deviceId));

    // Warmup
    writeOne(buffer, nullptr);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *buffer = 0u;

        writeOne(buffer, &timer);

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    omp_target_free(buffer, deviceId);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerSubmission> registerTestCase(run, Api::OMP);