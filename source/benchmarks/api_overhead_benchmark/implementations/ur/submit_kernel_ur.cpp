/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/utility/combo_profiler.h"
#include "framework/utility/file_helper.h"

#include "definitions/submit_kernel.h"

#include <gtest/gtest.h>

static constexpr size_t n_dimensions = 3;
static constexpr size_t global_size[] = {1, 1, 1};
static constexpr size_t local_size[] = {1, 1, 1};

static TestResult run(const SubmitKernelArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    int kernelExecutionTime = arguments.kernelExecutionTime;

    // Setup
    UrState ur;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.size() == 0)
        return TestResult::KernelNotFound;

    ur_program_handle_t program;
    EXPECT_UR_RESULT_SUCCESS(urProgramCreateWithIL(ur.context, spirvModule.data(),
                                                   spirvModule.size(), nullptr, &program));

    EXPECT_UR_RESULT_SUCCESS(urProgramBuild(ur.context, program, nullptr));

    const char *kernelName = "eat_time";

    ur_kernel_handle_t kernel;
    EXPECT_UR_RESULT_SUCCESS(urKernelCreate(program, kernelName, &kernel));

    ur_queue_handle_t queue;
    ur_queue_properties_t queueProperties = {};

    if (!arguments.inOrderQueue) {
        queueProperties.flags = UR_QUEUE_FLAG_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }

    EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                           &queueProperties, &queue));

    std::vector<ur_event_handle_t> events(arguments.numKernels);

    // warmup
    for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
        EXPECT_UR_RESULT_SUCCESS(urKernelSetArgValue(
            kernel, 0, sizeof(int), nullptr,
            reinterpret_cast<void *>(&kernelExecutionTime)));

        ur_event_handle_t *signalEvent = nullptr;
        if (arguments.useEvents) {
            signalEvent = &events[iteration];
        }

        EXPECT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(
            queue, kernel, n_dimensions, nullptr, &local_size[0],
            &global_size[0], 0, nullptr, 0, nullptr, signalEvent));
    }

    EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));

    for (auto &event : events) {
        if (event)
            urEventRelease(event);
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        profiler.measureStart();
        for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
            EXPECT_UR_RESULT_SUCCESS(urKernelSetArgValue(
                kernel, 0, sizeof(int), nullptr,
                reinterpret_cast<void *>(&kernelExecutionTime)));

            ur_event_handle_t *signalEvent = nullptr;
            if (arguments.useEvents) {
                signalEvent = &events[iteration];
            }

            EXPECT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(
                queue, kernel, n_dimensions, nullptr,
                &local_size[0], &global_size[0], 0, nullptr, 0, nullptr, signalEvent));
        }

        if (!arguments.measureCompletionTime) {
            profiler.measureEnd();
        }

        EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));

        if (arguments.measureCompletionTime) {
            profiler.measureEnd();
        }
        profiler.pushStats(statistics);

        for (auto &event : events) {
            if (event)
                urEventRelease(event);
        }
    }

    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queue));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernel));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernel> registerTestCase(run, Api::UR);
