/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/empty_kernels_with_global_timer.h"

#include <gtest/gtest.h>

static TestResult run(const EmptyKernelsWithGlobalTimerArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const size_t gws = 1u;
    const size_t lws = 1u;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("ulls_benchmark_empty_kernel.spv");
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
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));

    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    // Create signal events
    ze_event_pool_flags_t flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, flags, static_cast<uint32_t>(arguments.kernelCount)};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));

    std::vector<ze_event_handle_t> events(arguments.kernelCount);
    for (auto i = 0u; i < arguments.kernelCount; i++) {
        ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, i, 0, 0};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &events[i]));
    }

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    // Setup global timestamps
    const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    const auto timestampBufferSize = sizeof(uint64_t) * 2;
    void *timestampBuffer = nullptr;
    uint64_t *beginTimestamp = nullptr;
    uint64_t *endTimestamp = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, timestampBufferSize, 0, &timestampBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, timestampBuffer, timestampBufferSize))
    beginTimestamp = static_cast<uint64_t *>(timestampBuffer);
    endTimestamp = beginTimestamp + 1;

    // Setup command list
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
    for (auto i = 0u; i < arguments.kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, events[i], 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0u, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    for (auto i = 0u; i < arguments.kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
    }

    for (auto i = 0u; i < arguments.iterations; i++) {
        // Benchmark
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        auto totalTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
        totalTime *= timerResolution;
        statistics.pushValue(totalTime, typeSelector.getUnit(), MeasurementType::Gpu);
        for (auto j = 0u; j < arguments.kernelCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[j]));
        }
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, timestampBuffer, timestampBufferSize))
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestampBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (auto &hEvent : events) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EmptyKernelsWithGlobalTimer> registerTestCase(run, Api::L0);
