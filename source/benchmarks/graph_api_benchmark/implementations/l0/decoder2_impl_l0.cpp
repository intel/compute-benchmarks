/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "decoder2_impl_l0.h"

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/test_case.h"
#include "framework/test_case/test_result.h"

#include <cstdint>
#include <level_zero/ze_api.h>
#include <level_zero/zer_api.h>
#include <limits>

Decoder2GraphL0::DataIntPtr Decoder2GraphL0::allocDevice(uint32_t count) {
    void *ptr = nullptr;
    size_t bytes = count * sizeof(int);
    EXPECT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero->context, &zeDefaultGPUDeviceMemAllocDesc, bytes, 1, levelzero->device, &ptr));
    clearDeviceBuffer(static_cast<int *>(ptr), count);
    return DataIntPtr(static_cast<int *>(ptr), [levelzero = this->levelzero](int *ptr) {
        zeMemFree(levelzero->context, ptr);
    });
}

TestResult Decoder2GraphL0::clearDeviceBuffer(int *devicePtr, uint32_t count) {
    size_t bytes = count * sizeof(int);
    int val = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(immCmdList, devicePtr, &val, sizeof(int), bytes, nullptr, 0, nullptr));
    waitCompletion();
    return TestResult::Success;
}

TestResult Decoder2GraphL0::init() {
    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setGraphFunctions(!emulateGraphs);
    levelzero = std::make_shared<LevelZero>(extensionProperties);
    TestResult kernelDecoder2Res = L0::KernelHelper::loadKernel(*levelzero, "graph_api_benchmark_kernel_increment.spv", "kernel_increment", &kernelDecoder2, &moduleDecoder2);
    if (kernelDecoder2Res != TestResult::Success)
        return kernelDecoder2Res;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernelDecoder2, 1u, 1u, 1u));

    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListCreateImmediate(levelzero->context, levelzero->device,
                                     &zeDefaultGPUImmediateCommandQueueDesc, &immCmdList));

    if (useGraphs && !emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.graphCreate(levelzero->context, &graph, nullptr));
    }

    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero->commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero->context, levelzero->device, &cmdListDesc, &cmdList));
    return TestResult::Success;
}

TestResult Decoder2GraphL0::readResults(int *actualSum, int *actualSignalCount) {
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(
        immCmdList, actualSum, graphData.get(), size * sizeof(int),
        nullptr, 0, nullptr));
    waitCompletion();
    *actualSignalCount = *(com.canBegin);
    return TestResult::Success;
}

TestResult Decoder2GraphL0::destroy() {
    if (cmdList != nullptr)
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (useGraphs && !emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.executableGraphDestroy(execGraph));
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.graphDestroy(graph));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelDecoder2));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(moduleDecoder2));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(immCmdList));
    return TestResult::Success;
}

TestResult Decoder2GraphL0::runLayer() {
    uint32_t numIncrements = INCREMENTS_PER_KERNEL;
    int *data = graphData.get();
    for (uint32_t i = 0; i < KERNELS_PER_LAYER; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernelDecoder2, 0, sizeof(uint32_t), &numIncrements));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernelDecoder2, 1, sizeof(int *), &data));
        if (useGraphs && emulateGraphs) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernelDecoder2, &groupCount,
                                                                     nullptr, 0, nullptr));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(immCmdList, kernelDecoder2, &groupCount,
                                                                     nullptr, 0, nullptr));
        }
    }
    if (useGraphs && emulateGraphs)
        zeCommandListClose(cmdList);
    return TestResult::Success;
}

TestResult Decoder2GraphL0::runAllLayers() {
    for (uint32_t layerIdx = 0; layerIdx < LAYER_NUM; ++layerIdx) {
        runLayer();
    }
    return TestResult::Success;
}

TestResult Decoder2GraphL0::recordGraph() {
    if (useGraphs && !emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.commandListBeginCaptureIntoGraph(immCmdList, graph, nullptr));
        runLayer();
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.commandListEndGraphCapture(immCmdList, &graph, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.commandListInstantiateGraph(graph, &execGraph, nullptr));
    } else if (useGraphs) { // Emulation mode
        runLayer();
    }
    return TestResult::Success;
}

TestResult Decoder2GraphL0::waitCompletion() {
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
        immCmdList, std::numeric_limits<uint64_t>::max()));
    return TestResult::Success;
}

TestResult Decoder2GraphL0::runGraph() {
    if (useGraphs && !emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero->graphExtension.commandListAppendGraph(immCmdList, execGraph, nullptr,
                                                                                  nullptr, 0, nullptr));
    } else if (useGraphs && emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(immCmdList, 1, &cmdList, nullptr, 0, nullptr));
    } else { // Eager execution
        runLayer();
    }
    return TestResult::Success;
}

bool Decoder2GraphL0::isUnsupported() {
    // L0 does not support host tasks and graphs must be enabled to emulate them
    return useHostTasks || (!useGraphs && emulateGraphs);
}
