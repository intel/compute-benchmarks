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

    ur_queue_handle_t queue;
    ur_queue_properties_t queueProperties = {};

    auto inOrder = arguments.inOrderQueue;
    if (!inOrder) {
        queueProperties.flags = UR_QUEUE_FLAG_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }

    EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                           &queueProperties, &queue));

    ur_exp_command_buffer_handle_t cmdBuffer = nullptr;

    // Finalize the graph
    ur_exp_command_buffer_desc_t cmdBufferDesc = {
        UR_STRUCTURE_TYPE_EXP_COMMAND_BUFFER_DESC,
        nullptr, // pNext
        false,   // isUpdatable
        inOrder, // isInOrder
        false    // enableProfiling
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
    EXPECT_UR_RESULT_SUCCESS(
        urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, nullptr));
    EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        EXPECT_UR_RESULT_SUCCESS(
            urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, nullptr));

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
        }

        EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));

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
