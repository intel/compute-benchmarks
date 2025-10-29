/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sin_kernel_impl_ur.h"

#include "framework/test_case/test_result.h"

#include "ur_api.h"

#include <iostream>
#include <math.h>

SinKernelGraphUR::DataFloatPtr SinKernelGraphUR::allocDevice(uint32_t count) {
    struct ur_usm_desc_t desc = {UR_STRUCTURE_TYPE_CONTEXT_PROPERTIES, nullptr, 0, 4};

    void *deviceptr = nullptr;
    EXPECT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(urstate->context, urstate->device, &desc, nullptr,
                                              count * sizeof(float), &deviceptr));

    auto copied = urstate;
    return SinKernelGraphUR::DataFloatPtr(static_cast<float *>(deviceptr), [copied](float *ptr) {
        urUSMFree(copied->context, ptr);
    });
}

SinKernelGraphUR::DataFloatPtr SinKernelGraphUR::allocHost(uint32_t count) {
    struct ur_usm_desc_t desc = {UR_STRUCTURE_TYPE_CONTEXT_PROPERTIES, nullptr, 0, 4};

    void *hostptr = nullptr;
    EXPECT_UR_RESULT_SUCCESS(urUSMHostAlloc(urstate->context, &desc, nullptr,
                                            count * sizeof(float), &hostptr));

    auto copied = urstate;
    return SinKernelGraphUR::DataFloatPtr(static_cast<float *>(hostptr), [copied](float *ptr) {
        urUSMFree(copied->context, ptr);
    });
}

TestResult SinKernelGraphUR::init() {
    urstate = std::make_shared<UrState>();

    ur_queue_properties_t queueProperties{UR_STRUCTURE_TYPE_QUEUE_PROPERTIES};
    EXPECT_UR_RESULT_SUCCESS(
        urQueueCreate(urstate->context, urstate->device, &queueProperties, &queue));

    // check if the device supports command buffers

    ur_bool_t command_buffer_support = false;
    EXPECT_UR_RESULT_SUCCESS(urDeviceGetInfo(
        urstate->device, UR_DEVICE_INFO_COMMAND_BUFFER_SUPPORT_EXP,
        sizeof(command_buffer_support), &command_buffer_support, nullptr));
    if (!command_buffer_support) {
        return TestResult::DeviceNotCapable;
    }

    // Create kernels
    auto spirvModuleA =
        FileHelper::loadBinaryFile("graph_api_benchmark_kernel_assign.spv");
    auto spirvModuleS =
        FileHelper::loadBinaryFile("graph_api_benchmark_kernel_sin.spv");

    if (spirvModuleA.size() == 0 || spirvModuleS.size() == 0)
        return TestResult::KernelNotFound;

    char opts[] = "-ze-intel-enable-auto-large-GRF-mode -ze-opt-level=2";
    ur_exp_program_flags_t programFlags{};
    EXPECT_UR_RESULT_SUCCESS(urProgramCreateWithIL(urstate->context, spirvModuleA.data(),
                                                   spirvModuleA.size(), nullptr, &programA));
    EXPECT_UR_RESULT_SUCCESS(urProgramBuildExp(programA, 1, &urstate->device, programFlags, opts));
    EXPECT_UR_RESULT_SUCCESS(urKernelCreate(programA, "kernel_assign", &kernelAssign));

    EXPECT_UR_RESULT_SUCCESS(urProgramCreateWithIL(urstate->context, spirvModuleS.data(),
                                                   spirvModuleS.size(), nullptr, &programS));
    EXPECT_UR_RESULT_SUCCESS(urProgramBuildExp(programS, 1, &urstate->device, programFlags, opts));
    EXPECT_UR_RESULT_SUCCESS(urKernelCreate(programS, "kernel_sin", &kernelSin));

    return TestResult::Success;
}

TestResult SinKernelGraphUR::destroy() {
    if (cmdBuffer != nullptr)
        EXPECT_UR_RESULT_SUCCESS(urCommandBufferReleaseExp(cmdBuffer));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernelAssign));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernelSin));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(programA));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(programS));
    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queue));
    return TestResult::Success;
}

TestResult SinKernelGraphUR::runKernels() {
    size_t global_offset = 0;
    uint32_t n_dimensions = 1;
    size_t global_size[] = {size};

    float *dest = graphOutputData.get();
    float *source = graphInputData.get();

    EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernelAssign, 0, nullptr, dest));
    EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernelAssign, 1, nullptr, source));

    if (withGraphs) {
        assert(cmdBuffer != nullptr && "Command buffer is not initialized");
        EXPECT_UR_RESULT_SUCCESS(urCommandBufferAppendKernelLaunchExp(
            cmdBuffer, kernelAssign, n_dimensions, &global_offset, global_size,
            nullptr, 0, nullptr, 0, nullptr, 0, nullptr, &syncPoints[0], nullptr,
            nullptr));
    } else {
        assert(queue != nullptr && "Queue is not initialized");
        EXPECT_UR_RESULT_SUCCESS(
            urEnqueueKernelLaunch(queue, kernelAssign, n_dimensions, &global_offset,
                                  global_size, nullptr, 0, nullptr, 0, nullptr, nullptr));
    }

    for (uint32_t i = 0; i < numKernels; ++i) {
        std::swap(graphInputData, graphOutputData);

        dest = graphOutputData.get();
        source = graphInputData.get();

        EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernelSin, 0, nullptr, dest));
        EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernelSin, 1, nullptr, source));

        if (withGraphs) {
            EXPECT_UR_RESULT_SUCCESS(urCommandBufferAppendKernelLaunchExp(
                cmdBuffer, kernelSin, n_dimensions, &global_offset, global_size,
                nullptr, 0, nullptr, 1, &syncPoints[i], 0, nullptr,
                &syncPoints[i + 1], nullptr, nullptr));
        } else {
            EXPECT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(
                queue, kernelSin, n_dimensions, &global_offset, global_size,
                nullptr, 0, nullptr, 0, nullptr, nullptr));
        }
    }

    if (numKernels % 2 != 0) {
        std::swap(graphInputData, graphOutputData);
    }
    return TestResult::Success;
}

TestResult SinKernelGraphUR::recordGraph() {
    ur_exp_command_buffer_desc_t cmdBufferDesc = {
        UR_STRUCTURE_TYPE_EXP_COMMAND_BUFFER_DESC};
    cmdBufferDesc.isInOrder = true;
    cmdBufferDesc.isUpdatable = false;

    EXPECT_UR_RESULT_SUCCESS(urCommandBufferCreateExp(
        urstate->context, urstate->device, &cmdBufferDesc, &cmdBuffer));
    ASSERT_TEST_RESULT_SUCCESS(runKernels());
    EXPECT_UR_RESULT_SUCCESS(urCommandBufferFinalizeExp(cmdBuffer));

    return TestResult::Success;
}

TestResult SinKernelGraphUR::waitCompletion() {
    EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));

    return TestResult::Success;
}

TestResult SinKernelGraphUR::readResults(float *output_h) {
    EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, output_h, graphOutputData.get(), size * sizeof(float),
                                                0, nullptr, nullptr));
    EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));

    return TestResult::Success;
}

TestResult SinKernelGraphUR::runGraph(float *input_h) {
    // memcpy
    EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, graphInputData.get(),
                                                input_h, size * sizeof(float), 0,
                                                nullptr, nullptr));
    // run graph
    EXPECT_UR_RESULT_SUCCESS(
        urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, nullptr));

    return TestResult::Success;
}

TestResult SinKernelGraphUR::runEager(float *input_h) {
    EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, graphInputData.get(),
                                                input_h, size * sizeof(float), 0,
                                                nullptr, nullptr));
    // run kernels directly
    ASSERT_TEST_RESULT_SUCCESS(runKernels());

    return TestResult::Success;
}
