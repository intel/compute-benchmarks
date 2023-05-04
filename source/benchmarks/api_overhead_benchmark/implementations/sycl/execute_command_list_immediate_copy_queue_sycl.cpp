/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_immediate_copy_queue.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateCopyQueueArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Timer timer;
    Sycl sycl{sycl::device{sycl::gpu_selector_v}};

    // Create buffers
    auto srcBuffer = UsmHelper::allocate(arguments.sourcePlacement, sycl, arguments.size);
    auto dstBuffer = UsmHelper::allocate(arguments.destinationPlacement, sycl, arguments.size);

    // Warmup
    sycl.queue.memcpy(dstBuffer, srcBuffer, arguments.size).wait();

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        sycl.queue.memcpy(dstBuffer, srcBuffer, arguments.size);

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        sycl.queue.wait();

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Cleanup
    UsmHelper::deallocate(arguments.sourcePlacement, sycl, srcBuffer);
    UsmHelper::deallocate(arguments.destinationPlacement, sycl, dstBuffer);

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<ExecuteCommandListImmediateCopyQueue> registerTestCase(run, Api::SYCL);
