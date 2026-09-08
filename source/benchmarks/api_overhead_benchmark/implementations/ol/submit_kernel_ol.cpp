/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ol/error.h"
#include "framework/ol/ol.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"
#include "framework/utility/file_helper.h"

#include "definitions/submit_kernel.h"

#include <gtest/gtest.h>

// {dimensions, numGroups, groupSize, dynamicSharedMemory}: a single work item,
// matching the global/local size used by the UR and L0 implementations.
static constexpr ol_kernel_launch_size_args_t launchSizeArgs{
    3, {1, 1, 1}, {1, 1, 1}, 0};

static TestResult run(const SubmitKernelArguments &arguments,
                      Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    // Offload has no out-of-order queue equivalent.
    if (!arguments.inOrderQueue) {
        return TestResult::ApiNotCapable;
    }

    // OL_EVENT_FLAGS_ENABLE_PROFILING exists, but is not wired up here yet.
    if (arguments.useProfiling) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    int kernelExecutionTime = arguments.kernelExecutionTime;

    OlState ol;

    const auto spirvModule =
        FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.empty()) {
        return TestResult::KernelNotFound;
    }
    ol_program_handle_t program;
    ASSERT_OL_RESULT_SUCCESS(
        olCreateProgram(ol.context, ol.device, spirvModule.data(),
                        spirvModule.size(), &program));

    ol_symbol_handle_t kernel;
    ASSERT_OL_RESULT_SUCCESS(
        olGetSymbol(program, "eat_time", OL_SYMBOL_KIND_KERNEL, &kernel));

    ol_queue_handle_t queue;
    ASSERT_OL_RESULT_SUCCESS(olCreateQueue(ol.context, ol.device, &queue));

    std::vector<ol_event_handle_t> events(arguments.numKernels, nullptr);
    void *argPointers[] = {&kernelExecutionTime};
    const size_t argSizes[] = {sizeof(kernelExecutionTime)};

    for (auto i = 0u; i < arguments.iterations; ++i) {
        profiler.measureStart();
        for (auto iteration = 0u; iteration < arguments.numKernels;
             ++iteration) {
            ASSERT_OL_RESULT_SUCCESS(
                olLaunchKernel(queue, ol.device, kernel, &launchSizeArgs,
                               nullptr, 1, argPointers, argSizes));

            if (arguments.useEvents) {
                ASSERT_OL_RESULT_SUCCESS(
                    olCreateEvent(queue, OL_EVENT_FLAGS_NONE,
                                  &events[iteration]));
            }
        }

        if (!arguments.measureCompletionTime) {
            profiler.measureEnd();
        }

        ASSERT_OL_RESULT_SUCCESS(olSyncQueue(queue));

        if (arguments.measureCompletionTime) {
            profiler.measureEnd();
        }
        profiler.pushStats(statistics);

        for (auto &event : events) {
            if (event) {
                ASSERT_OL_RESULT_SUCCESS(olDestroyEvent(event));
                event = nullptr;
            }
        }
    }

    ASSERT_OL_RESULT_SUCCESS(olDestroyQueue(queue));
    ASSERT_OL_RESULT_SUCCESS(olDestroyProgram(program));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernel>
    registerTestCase(run, Api::OL);
