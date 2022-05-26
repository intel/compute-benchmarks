/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyKernelArguments &arguments, Statistics &statistics) {
    ContextProperties contextProperties = ContextProperties::create().create().setDeviceSelection(arguments.contextPlacement).allowCreationFail();
    QueueProperties queueProperties = QueueProperties::create().setDeviceSelection(arguments.queuePlacement).allowCreationFail();
    LevelZero levelzero(queueProperties, contextProperties);
    if (levelzero.context == nullptr || levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResolution(arguments.queuePlacement);

    // Create buffers
    void *src{}, *dst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.srcPlacement, levelzero, arguments.size, &src));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dstPlacement, levelzero, arguments.size, &dst));

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("multitile_memory_benchmark_copy_buffer.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.commandQueueDevice, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "copy_buffer";
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure dispath parameters
    uint32_t wgsX{}, wgsY{}, wgsZ{};
    const uint32_t elementsCount = static_cast<uint32_t>(arguments.size) / sizeof(int);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(kernel, elementsCount, 1u, 1u, &wgsX, &wgsY, &wgsZ));
    const uint32_t wgc = elementsCount / wgsX;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, wgsX, wgsY, wgsZ));
    const ze_group_count_t dispatchTraits{wgc, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(src), &src));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(dst), &dst));

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useEvents) {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.commandQueueDevice, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Make the buffers resident
    for (DeviceSelection device : DeviceSelectionHelper::split(DeviceSelectionHelper::withoutHost(arguments.queuePlacement | arguments.srcPlacement))) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(device), src, arguments.size));
    }
    for (DeviceSelection device : DeviceSelectionHelper::split(DeviceSelectionHelper::withoutHost(arguments.queuePlacement | arguments.dstPlacement))) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(device), dst, arguments.size));
    }

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.commandQueueDevice, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
            commandTime *= timerResolution;
            statistics.pushValue(commandTime, arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
        }
    }

    // Evict buffers
    for (DeviceSelection device : DeviceSelectionHelper::split(DeviceSelectionHelper::withoutHost(arguments.queuePlacement | arguments.srcPlacement))) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(device), src, arguments.size));
    }
    for (DeviceSelection device : DeviceSelectionHelper::split(DeviceSelectionHelper::withoutHost(arguments.queuePlacement | arguments.dstPlacement))) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(device), dst, arguments.size));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, src));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, dst));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyKernel> registerTestCase(run, Api::L0);
