/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/submit_graph.h"

#include <iostream>
#include <ur_api.h>

static constexpr size_t n_dimensions = 3;
static constexpr size_t global_size[] = {1, 1, 1};
static constexpr size_t local_size[] = {1, 1, 1};
static constexpr size_t global_offset = 0;

static TestResult run([[maybe_unused]] const SubmitGraphArguments &arguments, Statistics &statistics) {

    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    UrState ur;

    // check if the device supports command buffers
    ur_bool_t command_buffer_support = false;
    EXPECT_UR_RESULT_SUCCESS(urDeviceGetInfo(
        ur.device, UR_DEVICE_INFO_COMMAND_BUFFER_SUPPORT_EXP,
        sizeof(command_buffer_support), &command_buffer_support, nullptr));
    if (!command_buffer_support) {
        return TestResult::DeviceNotCapable;
    }

    Timer timer;

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
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);
    EXPECT_UR_RESULT_SUCCESS(urKernelSetArgValue(
        kernel, 0, sizeof(int), nullptr,
        reinterpret_cast<void *>(&kernelOperationsCount)));

    ur_queue_handle_t queue;
    ur_queue_properties_t queueProperties = {};

    ur_queue_flags_t queueFlags = 0;
    auto inOrder = arguments.inOrderQueue;
    if (!inOrder) {
        queueFlags |= UR_QUEUE_FLAG_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }
    auto useProfiling = arguments.useProfiling;
    if (useProfiling) {
        queueFlags |= UR_QUEUE_FLAG_PROFILING_ENABLE;
    }
    queueProperties.flags = queueFlags;

    EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                           &queueProperties, &queue));

    ur_exp_command_buffer_handle_t cmdBuffer = nullptr;

    // Finalize the graph
    ur_exp_command_buffer_desc_t cmdBufferDesc = {
        UR_STRUCTURE_TYPE_EXP_COMMAND_BUFFER_DESC,
        nullptr,     // pNext
        false,       // isUpdatable
        inOrder,     // isInOrder
        useProfiling // enableProfiling
    };

    EXPECT_UR_RESULT_SUCCESS(urCommandBufferCreateExp(
        ur.context, ur.device, &cmdBufferDesc, &cmdBuffer));
    for (auto i = 0u; i < arguments.numKernels; i++) {
        EXPECT_UR_RESULT_SUCCESS(urCommandBufferAppendKernelLaunchExp(
            cmdBuffer, kernel, n_dimensions, &global_offset, global_size,
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
            nullptr, nullptr, nullptr));
    }

    EXPECT_UR_RESULT_SUCCESS(urCommandBufferFinalizeExp(
        cmdBuffer));

    // Warmup
    if (!arguments.useEvents) {
        EXPECT_UR_RESULT_SUCCESS(
            urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, nullptr));
        EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));
    } else {
        ur_event_handle_t event;
        EXPECT_UR_RESULT_SUCCESS(
            urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, &event));
        EXPECT_UR_RESULT_SUCCESS(urEventWait(1, &event));
        EXPECT_UR_RESULT_SUCCESS(urEventRelease(event));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        if (!arguments.useEvents) {
            EXPECT_UR_RESULT_SUCCESS(
                urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, nullptr));

            if (!arguments.measureCompletionTime) {
                timer.measureEnd();
            }

            EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));
        } else {
            ur_event_handle_t event;
            EXPECT_UR_RESULT_SUCCESS(
                urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, &event));

            if (!arguments.measureCompletionTime) {
                timer.measureEnd();
            }

            EXPECT_UR_RESULT_SUCCESS(urEventWait(1, &event));
            EXPECT_UR_RESULT_SUCCESS(urEventRelease(event));
        }

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    EXPECT_UR_RESULT_SUCCESS(urCommandBufferReleaseExp(cmdBuffer));
    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queue));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernel));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitGraph> registerTestCase(run, Api::UR);
