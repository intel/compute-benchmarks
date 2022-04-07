/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_with_event.h"

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include <gtest/gtest.h>

static TestResult run(const KernelWithEventArguments &arguments, Statistics &statistics) {
    LevelZero levelzero;
    const uint64_t timerResolution = levelzero.getTimerResoultion(levelzero.device);

    // Create buffer
    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    const auto bufferSize = sizeof(uint64_t) * 3;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize))
    uint64_t *beginTimestamp = static_cast<uint64_t *>(buffer);
    uint64_t *endTimestamp = beginTimestamp + 1;

    // Create output buffer
    void *outputBuffer = nullptr;
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    const auto outputBufferSize = sizeof(uint32_t) * arguments.workgroupCount * arguments.workgroupSize;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, outputBufferSize, 0, levelzero.device, &outputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, outputBuffer, outputBufferSize))
    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("gpu_cmds_benchmark_empty_kernel.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "empty";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(arguments.workgroupSize), 1u, 1u));

    // Create event
    ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, 1};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0, 0};
    if (arguments.useHostSignalEvent) {
        eventPoolDesc.flags |= ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventDesc.signal |= ZE_EVENT_SCOPE_FLAG_HOST;
    }
    if (arguments.useDeviceWaitEvent) {
        eventDesc.wait |= ZE_EVENT_SCOPE_FLAG_DEVICE;
    }
    if (arguments.useTimestampEvent) {
        eventPoolDesc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    }

    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
    for (auto commandIndex = 0u; commandIndex < arguments.measuredCommands; commandIndex++) {
        const ze_group_count_t groupCount{static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0u, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Reset the event
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        auto commandTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
        commandTime *= timerResolution;
        commandTime /= arguments.measuredCommands;
        statistics.pushValue(commandTime, MeasurementUnit::Microseconds, MeasurementType::Gpu);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, outputBuffer, outputBufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, outputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelWithEvent> registerTestCase(run, Api::L0);
