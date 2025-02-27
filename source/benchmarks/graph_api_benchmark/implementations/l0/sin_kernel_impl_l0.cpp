/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sin_kernel_impl_l0.h"

#include "framework/l0/levelzero.h"
#include "framework/test_case/test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/file_helper.h"

#include <iostream>
#include <level_zero/ze_api.h>
#include <math.h>

typedef struct _zex_intel_queue_copy_operations_offload_hint_exp_desc_t {
    ze_structure_type_t stype;
    const void *pNext;
    ze_bool_t copyOffloadEnabled;
} zex_intel_queue_copy_operations_offload_hint_exp_desc_t;
#define ZEX_INTEL_STRUCTURE_TYPE_QUEUE_COPY_OPERATIONS_OFFLOAD_HINT_EXP_PROPERTIES (ze_structure_type_t)0x0003001B

SinKernelGraphL0::DataFloatPtr SinKernelGraphL0::allocDevice(uint32_t count) {
    void *deviceptr = nullptr;
    ze_device_mem_alloc_desc_t deviceAllocationDesc = {
        ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};

    EXPECT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero->context, &deviceAllocationDesc,
                                              count * sizeof(float), 0, levelzero->device,
                                              &deviceptr));

    auto copied = levelzero;
    return SinKernelGraphL0::DataFloatPtr(static_cast<float *>(deviceptr), [copied](float *ptr) {
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(copied->context, ptr));
    });
}

SinKernelGraphL0::DataFloatPtr SinKernelGraphL0::allocHost(uint32_t count) {
    void *hostptr = nullptr;
    ze_host_mem_alloc_desc_t hostAllocationDesc = {
        ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};

    EXPECT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero->context, &hostAllocationDesc,
                                            count * sizeof(float), 0, &hostptr));

    auto copied = levelzero;
    return SinKernelGraphL0::DataFloatPtr(static_cast<float *>(hostptr), [copied](float *ptr) {
        EXPECT_ZE_RESULT_SUCCESS(zeMemFree(copied->context, ptr));
    });
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

    auto spirvModuleA =
        FileHelper::loadBinaryFile("graph_api_benchmark_kernel_assign.spv");
    auto spirvModuleS =
        FileHelper::loadBinaryFile("graph_api_benchmark_kernel_sin.spv");

    if (spirvModuleA.size() == 0 || spirvModuleS.size() == 0) {
        return TestResult::KernelNotFound;
    }

    ze_module_desc_t moduleDescA{ZE_STRUCTURE_TYPE_MODULE_DESC};
    ze_module_desc_t moduleDescS{ZE_STRUCTURE_TYPE_MODULE_DESC};

    moduleDescA.format = moduleDescS.format = ZE_MODULE_FORMAT_IL_SPIRV;

    moduleDescA.pInputModule = reinterpret_cast<const uint8_t *>(spirvModuleA.data());
    moduleDescS.pInputModule = reinterpret_cast<const uint8_t *>(spirvModuleS.data());

    moduleDescA.inputSize = spirvModuleA.size();
    moduleDescS.inputSize = spirvModuleS.size();

    ze_kernel_desc_t kernelDescA{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    ze_kernel_desc_t kernelDescS{ZE_STRUCTURE_TYPE_KERNEL_DESC};

    kernelDescA.pKernelName = "kernel_assign";
    kernelDescS.pKernelName = "kernel_sin";

    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero->context, levelzero->device,
                                            &moduleDescA, &moduleAssign, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero->context, levelzero->device,
                                            &moduleDescS, &moduleSin, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(moduleAssign, &kernelDescA, &kernelAssign));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(moduleSin, &kernelDescS, &kernelSin));

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

SinKernelGraphL0::~SinKernelGraphL0() {
    if (levelzero == nullptr) // never inited, likely noop run
        return;

    if (graphCmdList != nullptr)
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(graphCmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelAssign));
    EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernelSin));
    EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(moduleAssign));
    EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(moduleSin));
    EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(zeEvent));
    EXPECT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(zePool));
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(immCmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(cmdQueue));
}
