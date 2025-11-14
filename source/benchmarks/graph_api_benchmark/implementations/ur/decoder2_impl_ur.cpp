/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "decoder2_impl_ur.h"

#include "framework/test_case/test_case.h"
#include "framework/ur/error.h"

Decoder2GraphUR::DataIntPtr Decoder2GraphUR::allocDevice(uint32_t count) {
    void *ptr = nullptr;
    size_t bytes = count * sizeof(int);
    ur_usm_desc_t desc = {UR_STRUCTURE_TYPE_CONTEXT_PROPERTIES, nullptr, 0, 4};

    EXPECT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(urState->context, urState->device, &desc, nullptr, bytes, &ptr));
    clearDeviceBuffer(static_cast<int *>(ptr), count);

    auto capturedUrState = urState;
    return DataIntPtr(static_cast<int *>(ptr), [capturedUrState](int *ptr) {
        urUSMFree(capturedUrState->context, ptr);
    });
}

TestResult Decoder2GraphUR::clearDeviceBuffer(int *devicePtr, uint32_t count) {
    size_t bytes = count * sizeof(int);
    uint32_t pattern = 0;

    EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMFill(queue, devicePtr, sizeof(uint32_t), &pattern, bytes, 0, nullptr, nullptr));
    ASSERT_TEST_RESULT_SUCCESS(waitCompletion());

    return TestResult::Success;
}

TestResult Decoder2GraphUR::init() {
    urState = std::make_shared<UrState>();

    // Check if device supports command buffers
    ur_bool_t commandBufferSupport = false;
    EXPECT_UR_RESULT_SUCCESS(urDeviceGetInfo(
        urState->device, UR_DEVICE_INFO_COMMAND_BUFFER_SUPPORT_EXP,
        sizeof(commandBufferSupport), &commandBufferSupport, nullptr));
    if (!commandBufferSupport && useGraphs) {
        return TestResult::DeviceNotCapable;
    }

    ur_queue_properties_t queueProperties = {};
    EXPECT_UR_RESULT_SUCCESS(urQueueCreate(urState->context, urState->device, &queueProperties, &queue));

    auto spirvModule = FileHelper::loadBinaryFile("graph_api_benchmark_kernel_increment.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }

    const char *kernelName = "kernel_increment";

    EXPECT_UR_RESULT_SUCCESS(urProgramCreateWithIL(urState->context, spirvModule.data(), spirvModule.size(), nullptr, &program));
    EXPECT_UR_RESULT_SUCCESS(urProgramBuild(urState->context, program, nullptr));
    EXPECT_UR_RESULT_SUCCESS(urKernelCreate(program, kernelName, &kernel));

    // Create command buffer if using graphs
    if (useGraphs) {
        ur_exp_command_buffer_desc_t cmdBufferDesc = {
            UR_STRUCTURE_TYPE_EXP_COMMAND_BUFFER_DESC,
            nullptr, // pNext
            false,   // isUpdatable
            true,    // isInOrder
            false    // enableProfiling
        };
        EXPECT_UR_RESULT_SUCCESS(urCommandBufferCreateExp(urState->context, urState->device, &cmdBufferDesc, &cmdBuffer));
    }
    return TestResult::Success;
}

TestResult Decoder2GraphUR::destroy() {
    if (useGraphs) {
        EXPECT_UR_RESULT_SUCCESS(urCommandBufferReleaseExp(cmdBuffer));
    }
    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queue));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernel));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));

    return TestResult::Success;
}

TestResult Decoder2GraphUR::readResults(int *actualSum, int *actualSignalCount) {
    EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, true, actualSum, graphData.get(), sizeof(int), 0, nullptr, nullptr));
    *actualSignalCount = *(com.canBegin);

    return TestResult::Success;
}

bool Decoder2GraphUR::isUnsupported() {
    return useHostTasks || (useGraphs ^ emulateGraphs);
}

TestResult Decoder2GraphUR::runLayer() {
    uint32_t numIncrements = INCREMENTS_PER_KERNEL;
    int *data = graphData.get();

    // Launch kernels for this layer
    for (uint32_t i = 0; i < KERNELS_PER_LAYER; ++i) {
        EXPECT_UR_RESULT_SUCCESS(urKernelSetArgValue(kernel, 0, sizeof(uint32_t), nullptr, &numIncrements));
        EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernel, 1, nullptr, data));
        if (useGraphs) {
            // Append to command buffer
            EXPECT_UR_RESULT_SUCCESS(urCommandBufferAppendKernelLaunchExp(
                cmdBuffer, kernel, nDimensions, &globalOffset, globalWorkGroupSize,
                nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
                nullptr, nullptr, nullptr));
        } else {
            // Eager submission
            EXPECT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(
                queue, kernel, nDimensions, &globalOffset, globalWorkGroupSize,
                nullptr, nullptr, 0, nullptr, nullptr));
        }
    }

    return TestResult::Success;
}

TestResult Decoder2GraphUR::runAllLayers() {
    for (uint32_t layerIdx = 0; layerIdx < LAYER_NUM; ++layerIdx) {
        ASSERT_TEST_RESULT_SUCCESS(runLayer());
    }

    return TestResult::Success;
}

TestResult Decoder2GraphUR::recordGraph() {
    if (useGraphs) {
        ASSERT_TEST_RESULT_SUCCESS(runLayer());
        EXPECT_UR_RESULT_SUCCESS(urCommandBufferFinalizeExp(cmdBuffer));
    }

    return TestResult::Success;
}

TestResult Decoder2GraphUR::waitCompletion() {
    EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));
    return TestResult::Success;
}

TestResult Decoder2GraphUR::runGraph() {
    if (useGraphs) {
        EXPECT_UR_RESULT_SUCCESS(urEnqueueCommandBufferExp(queue, cmdBuffer, 0, nullptr, nullptr));
    } else {
        ASSERT_TEST_RESULT_SUCCESS(runLayer());
    }

    return TestResult::Success;
}
