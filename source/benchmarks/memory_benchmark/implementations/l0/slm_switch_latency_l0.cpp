/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"

#include "definitions/slm_switch_latency.h"

#include <gtest/gtest.h>

using namespace MemoryConstants;

static TestResult run(const SlmSwitchLatencyArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties = QueueProperties::create();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create();

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);

    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    const uint32_t kernelCount = 2;

    const size_t bufferSize = 1024 * kiloByte;

    // Create module
    const char *kernelFile = "slm_benchmark.spv";
    auto spirvModule = FileHelper::loadBinaryFile(kernelFile);
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));

    // Create buffer
    void *buffers[kernelCount];
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    for (auto i = 0u; i < kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffers[i]));
    }

    // Configure kernel group size
    const ze_group_count_t dispatchTraits{1, 1u, 1u};

    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    // Create kernel
    size_t slmSizes[2] = {arguments.slmPerWkgKernel1, arguments.slmPerWkgKernel2};
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    int operations = 1000;
    ze_kernel_handle_t kernels[kernelCount];
    for (auto i = 0u; i < kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], arguments.wgs, 1u, 1u));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(operations), &operations));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 1, sizeof(buffers[i]), &buffers[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 2, slmSizes[i], nullptr));
    }

    // Create events for profiling
    ze_event_pool_flags_t flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;

    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, flags, static_cast<uint32_t>(kernelCount)};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));

    std::vector<ze_event_handle_t> profilingEvents(kernelCount);

    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, 0, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &profilingEvents[0]));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[0], &dispatchTraits, profilingEvents[0], 0, nullptr));

    for (auto i = 1u; i < kernelCount; i++) {
        ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, i, 0, 0};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &profilingEvents[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[i], &dispatchTraits, profilingEvents[i], 1, &profilingEvents[i - 1]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    for (auto j = 0u; j < kernelCount; j++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvents[j]));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        // Launch kernel
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, 0));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        ze_kernel_timestamp_result_t earlierKernelTimestamp;
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[0], &earlierKernelTimestamp));
        ze_kernel_timestamp_result_t laterKernelTimestamp;
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvents[1], &laterKernelTimestamp));
        auto switchTime = std::chrono::nanoseconds((laterKernelTimestamp.global.kernelStart - earlierKernelTimestamp.global.kernelEnd) * timerResolution);

        statistics.pushValue(switchTime, MeasurementUnit::Microseconds, MeasurementType::Gpu);

        for (auto j = 0u; j < kernelCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvents[j]));
        }
    }

    // Cleanup
    for (auto i = 0u; i < kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffers[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<SlmSwitchLatency> registerTestCase(run, Api::L0);
