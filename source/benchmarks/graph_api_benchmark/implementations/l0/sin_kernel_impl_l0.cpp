/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sin_kernel_impl_l0.h"

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/file_helper.h"

#include "implementations/l0/memory_helper.h"

#include <iostream>
#include <level_zero/ze_api.h>
#include <math.h>

SinKernelGraphL0::DataFloatPtr SinKernelGraphL0::allocDevice(uint32_t count) {
    return mem_helper::alloc(UsmMemoryPlacement::Device, levelzero, count);
}

SinKernelGraphL0::DataFloatPtr SinKernelGraphL0::allocHost(uint32_t count) {
    return mem_helper::alloc(UsmMemoryPlacement::Host, levelzero, count);
}

TestResult SinKernelGraphL0::runKernels(ze_command_list_handle_t cmdList) {
    float *dest = graphOutputData.get();
    float *source = graphInputData.get();

    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(kernelAssign, 0, sizeof(float *), &dest));
    ASSERT_ZE_RESULT_SUCCESS(
        zeKernelSetArgumentValue(kernelAssign, 1, sizeof(float *), &source));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
        cmdList, kernelAssign, &groupCount, nullptr, 0, nullptr));

    for (uint32_t i = 0; i < numKernels; ++i) {
        std::swap(graphInputData, graphOutputData);

        dest = graphOutputData.get();
        source = graphInputData.get();

        ASSERT_ZE_RESULT_SUCCESS(
            zeKernelSetArgumentValue(kernelSin, 0, sizeof(float *), &dest));
        ASSERT_ZE_RESULT_SUCCESS(
            zeKernelSetArgumentValue(kernelSin, 1, sizeof(float *), &source));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
            cmdList, kernelSin, &groupCount, nullptr, 0, nullptr));
    }

    if (numKernels % 2 != 0) {
        std::swap(graphInputData, graphOutputData);
    }
    return TestResult::Success;
}

TestResult SinKernelGraphL0::init() {
    levelzero = std::make_shared<LevelZero>();

    TestResult kernelA_res = L0::KernelHelper::loadKernel(*levelzero, "graph_api_benchmark_kernel_assign.spv", "kernel_assign", &kernelAssign, &moduleAssign);
    if (kernelA_res != TestResult::Success) {
        return kernelA_res;
    }
    TestResult kernelB_res = L0::KernelHelper::loadKernel(*levelzero, "graph_api_benchmark_kernel_sin.spv", "kernel_sin", &kernelSin, &moduleSin);
    if (kernelB_res != TestResult::Success) {
        return kernelB_res;
    }

    uint32_t grpCnt[3] = {1, 1, 1};

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(kernelAssign, size, 1, 1, grpCnt,
                                                      grpCnt + 1, grpCnt + 2));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernelAssign, grpCnt[0], grpCnt[1], grpCnt[2]));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(kernelSin, size, 1, 1, grpCnt,
                                                      grpCnt + 1, grpCnt + 2));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernelSin, grpCnt[0], grpCnt[1], grpCnt[2]));

    zex_intel_queue_copy_operations_offload_hint_exp_desc_t copyOffload = {ZEX_INTEL_STRUCTURE_TYPE_QUEUE_COPY_OPERATIONS_OFFLOAD_HINT_EXP_PROPERTIES, nullptr, true};

    ze_command_queue_desc_t cmdQueueDesc = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    cmdQueueDesc.ordinal = levelzero->commandQueueDesc.ordinal;
    cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    cmdQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    cmdQueueDesc.pNext = withCopyOffload ? &copyOffload : nullptr;

    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListCreateImmediate(levelzero->context, levelzero->device,
                                     &cmdQueueDesc, &immCmdList));

    // copy offload is only supported for immediate command lists
    cmdQueueDesc.pNext = nullptr;
    cmdQueueDesc.flags = 0;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero->context, levelzero->device,
                                                  &cmdQueueDesc, &cmdQueue))

    ze_event_pool_desc_t pdesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    pdesc.count = 1;
    pdesc.flags = 0;
    pdesc.pNext = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero->context, &pdesc, 1, &levelzero->device, &zePool));

    ze_event_desc_t edesc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
    edesc.index = 0;
    edesc.signal = 0;
    edesc.wait = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(zePool, &edesc, &zeEvent));
    return TestResult::Success;
}

TestResult SinKernelGraphL0::destroy() {
    if (graphCmdList != nullptr)
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(graphCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelAssign));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelSin));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(moduleAssign));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(moduleSin));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(zeEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(zePool));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(immCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(cmdQueue));
    return TestResult::Success;
}

TestResult SinKernelGraphL0::recordGraph() {
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};

    cmdListDesc.commandQueueGroupOrdinal = levelzero->commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(
        levelzero->context, levelzero->device, &cmdListDesc, &graphCmdList));

    if (!immediateAppendCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(graphCmdList, 1, &zeEvent));
    }

    runKernels(graphCmdList);

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(graphCmdList));

    return TestResult::Success;
}

TestResult SinKernelGraphL0::readResults(float *output_h) {
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(
        immCmdList, output_h, graphOutputData.get(), size * sizeof(float),
        nullptr, 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
        immCmdList, std::numeric_limits<uint64_t>::max()));

    return TestResult::Success;
}

TestResult SinKernelGraphL0::runGraph(float *input_h) {
    ze_event_handle_t event = immediateAppendCmdList ? nullptr : zeEvent;
    // memcpy
    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListAppendMemoryCopy(immCmdList, graphInputData.get(), input_h,
                                      size * sizeof(float), event, 0, nullptr));

    // run graph
    if (immediateAppendCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
            immCmdList, 1, &graphCmdList, nullptr, 0, nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(cmdQueue, 1, &graphCmdList, nullptr));
    }

    return TestResult::Success;
}

TestResult SinKernelGraphL0::runEager(float *input_h) {
    // memcpy
    ASSERT_ZE_RESULT_SUCCESS(
        zeCommandListAppendMemoryCopy(immCmdList, graphInputData.get(), input_h,
                                      size * sizeof(float), nullptr, 0, nullptr));

    // run kernels directly
    ASSERT_TEST_RESULT_SUCCESS(runKernels(immCmdList));

    return TestResult::Success;
}

TestResult SinKernelGraphL0::waitCompletion() {
    if (!immediateAppendCmdList && withGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(cmdQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(zeEvent));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
            immCmdList, std::numeric_limits<uint64_t>::max()));
    }

    return TestResult::Success;
}
