/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/utility/linux/set_env_raii.h"
#include "framework/utility/timer.h"

#include "definitions/empty_kernel.h"

#include <omp.h>

void empty(const size_t wgc, const size_t lws) {
#pragma omp target teams distribute parallel for num_teams(wgc) thread_limit(lws)
    {
        for (size_t i = 0; i < wgc * lws; ++i) {
        }
    }
}

static TestResult run(const EmptyKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    SetEnvRAII("OMP_TARGET_OFFLOAD", "MANDATORY");
    Timer timer;
    const size_t wgc = arguments.workgroupCount;
    const size_t lws = arguments.workgroupSize;

    // Warmup
    empty(wgc, lws);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        empty(wgc, lws);

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<EmptyKernel> registerTestCase(run, Api::OMP);
