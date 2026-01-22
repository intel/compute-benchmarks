/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/barrier_between_kernels.h"

#include <gtest/gtest.h>

static TestResult run(const BarrierBetweenKernelsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    levelzero.createSubDevices(false, true);
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    if (arguments.remoteAccess && levelzero.getSubDevicesCount() < 2) {
        return TestResult::DeviceNotCapable;
    }
    if (arguments.remoteAccess && arguments.flushedMemory == UsmMemoryPlacement::Host) {
        return TestResult::DeviceNotCapable;
    }

    // Create timestamp buffer
    ze_host_mem_alloc_desc_t timestampHostDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    timestampHostDesc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED;
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *timestampBuffer = nullptr;
    const auto timestampBufferSize = sizeof(uint64_t) * 100;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &timestampHostDesc, timestampBufferSize, 0, &timestampBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, timestampBuffer, timestampBufferSize))
    uint64_t *beginTimestamp = static_cast<uint64_t *>(timestampBuffer);
    uint64_t *endTimestamp = beginTimestamp + 1;
    uint64_t *meassurmentCostStart = endTimestamp + 1;
    uint64_t *meassurmentCostEnd = meassurmentCostStart + 1;

    // Create output buffer
    void *outputBuffer = nullptr;
    const auto outputBufferSize = arguments.bytesToFlush;
    if (arguments.flushedMemory == UsmMemoryPlacement::Device) {
        if (arguments.remoteAccess) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, outputBufferSize, 0, levelzero.getDevice(DeviceSelection::Tile1), &outputBuffer));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, outputBufferSize, 0, levelzero.getDevice(DeviceSelection::Tile0), &outputBuffer));
        }
    } else {
        ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, outputBufferSize, 0, &outputBuffer));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, outputBuffer, outputBufferSize));

    {
        // Initialize buffer data
        ze_command_list_desc_t cmdListDesc{};
        cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
        ze_command_list_handle_t cmdListForFill{};

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdListForFill));
        uint32_t pattern = arguments.onlyReads == 0 ? 1u : 0u;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdListForFill, outputBuffer, &pattern, sizeof(uint32_t), outputBufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdListForFill));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdListForFill, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    }

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("gpu_cmds_benchmark_write_one_global_ids_with_check.spv");
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

    if (arguments.onlyReads) {
        kernelDesc.pKernelName = "write_one";
    } else {
        kernelDesc.pKernelName = "only_write_one";
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    auto sizeInDwords = outputBufferSize / sizeof(uint32_t);

    auto workgroupSize = sizeInDwords > 32 ? 32 : sizeInDwords;
    auto workgroupCount = sizeInDwords / workgroupSize;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(workgroupSize), 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(outputBuffer), &outputBuffer));

    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    const ze_group_count_t groupCount{static_cast<uint32_t>(workgroupCount), 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0u, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, event, 0u, nullptr));
    if (arguments.barrierCount > 1) {
        for (auto barrierId = 1u; barrierId < arguments.barrierCount; barrierId++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0u, nullptr));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, meassurmentCostStart, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, meassurmentCostEnd, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        auto commandTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
        auto meassureTime = std::chrono::nanoseconds(*meassurmentCostEnd - *meassurmentCostStart);
        if (commandTime >= meassureTime) {
            commandTime -= meassureTime;
        }

        commandTime *= timerResolution;
        statistics.pushValue(commandTime, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, outputBuffer, outputBufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, outputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, timestampBuffer, timestampBufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestampBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<BarrierBetweenKernels> registerTestCase(run, Api::L0);
