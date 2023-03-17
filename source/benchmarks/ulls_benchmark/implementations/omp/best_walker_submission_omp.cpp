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

void writeOne(uint64_t *buffer) {
#pragma omp target nowait
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
    auto buffer = static_cast<uint64_t *>(omp_target_alloc_host(sizeof(uint64_t), deviceId));

    // Warmup
    writeOne(buffer);
#pragma omp taskwait

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *buffer = 0u;

        timer.measureStart();

        writeOne(buffer);
        while (*static_cast<volatile uint64_t *>(buffer) != 1u) {
        }

        timer.measureEnd();

#pragma omp taskwait
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    omp_target_free(buffer, deviceId);

    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerSubmission> registerTestCase(run, Api::OMP);
