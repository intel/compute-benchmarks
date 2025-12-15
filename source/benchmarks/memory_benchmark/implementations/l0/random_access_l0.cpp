/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/measurement_type.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/error.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/random_access.h"
#include "level_zero/ze_api.h"

#include <gtest/gtest.h>
#include <random>

enum AccessMode {
    Read = 0u,
    Write,
    ReadWrite
};

static AccessMode getAccessMode(std::string accessModeString) {
    if (accessModeString == "Read" || accessModeString == "READ") {
        return AccessMode::Read;
    }

    if (accessModeString == "Write" || accessModeString == "WRITE") {
        return AccessMode::Write;
    }

    if (accessModeString == "ReadWrite" || accessModeString == "READWRITE") {
        return AccessMode::ReadWrite;
    }

    return AccessMode::Read;
}

static TestResult run(const RandomAccessArguments &arguments, Statistics &statistics) {
    const size_t iterations = arguments.iterations;
    const size_t allocationSize = arguments.allocationSize;
    const size_t alignment = arguments.alignment;
    const std::string accessModeArg = arguments.accessMode;
    const size_t randomAccessRange = arguments.randomAccessRange;
    const MeasurementType measurementType = arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu;

    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, measurementType);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;

    ze_device_compute_properties_t computeProperties{ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetComputeProperties(levelzero.device, &computeProperties));

    const uint32_t workItemCnt = 10 * MemoryConstants::megaByte;
    const size_t srcBufferAccessElementSize = sizeof(uint32_t);
    const size_t offsetAccessBytesPerThread = sizeof(uint32_t);

    ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    if (deviceProperties.maxMemAllocSize < allocationSize) {
        return TestResult::DeviceNotCapable;
    }

    ze_device_memory_properties_t memProperties{ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES};
    uint32_t memPropertiesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryProperties(levelzero.device, &memPropertiesCount, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryProperties(levelzero.device, &memPropertiesCount, &memProperties));

    // Consider the 3 allocations used in the benchmark and additional size due to alignment requirements
    const uint64_t maxMemoryRequiredByBenchmark = allocationSize + workItemCnt * offsetAccessBytesPerThread + 1 +
                                                  alignment * 3u;
    if (memPropertiesCount == 0 || memProperties.totalSize < maxMemoryRequiredByBenchmark) {
        return TestResult::DeviceNotCapable;
    }

    if ((allocationSize / srcBufferAccessElementSize) > pow(2, offsetAccessBytesPerThread * 8u)) {
        return TestResult::InvalidArgs;
    }

    // Create buffer
    const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};

    void *srcBuffer{}, *offsetBuffer{}, *result;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, allocationSize, alignment, levelzero.device, &srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, 1, alignment, levelzero.device, &result));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(levelzero.context, &deviceAllocationDesc, &hostAllocationDesc, workItemCnt * offsetAccessBytesPerThread, alignment, levelzero.device, &offsetBuffer));

    const double maxPossibleAccessIndex = static_cast<double>((allocationSize / srcBufferAccessElementSize) - 1);

    // Prepare Offset bufffer
    uint32_t *randBuff = reinterpret_cast<uint32_t *>(offsetBuffer);
    std::mt19937 generator(static_cast<uint32_t>(arguments.randomAccessSeed));
    std::uniform_int_distribution<uint32_t> distr(0, static_cast<uint32_t>(maxPossibleAccessIndex) - 1);
    for (auto index = 0u; index < workItemCnt; index++) {
        randBuff[index] = distr(generator);
    }

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("access_device_mem_random.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{
        ZE_STRUCTURE_TYPE_MODULE_DESC,
        nullptr,
        ZE_MODULE_FORMAT_IL_SPIRV,
        kernelBinary.size(),
        kernelBinary.data()};
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{
        ZE_STRUCTURE_TYPE_KERNEL_DESC,
        nullptr,
        ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY,
        "access_device_memory_random"};
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure dispatch parameters
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, computeProperties.maxGroupSizeX, 1, 1));
    const ze_group_count_t dispatchTraits{workItemCnt / computeProperties.maxGroupSizeX, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(srcBuffer), &srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(randBuff), &randBuff));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(result), &result));

    const uint32_t accessMode = getAccessMode(accessModeArg);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, sizeof(accessMode), &accessMode));
    const uint32_t maxAccessIndex = std::min<uint32_t>(static_cast<uint32_t>(randomAccessRange), 100u) * static_cast<uint32_t>(maxPossibleAccessIndex / 100.0);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 4, sizeof(maxAccessIndex), &maxAccessIndex));

    // Create command list
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    // Timer setup for cpu measurement
    Timer timer;

    // Create event for gpu time measurement
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useEvents) {
        ze_event_pool_desc_t eventPoolDesc{
            ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
            nullptr,
            ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP,
            1};

        ze_event_desc_t eventDesc{
            ZE_STRUCTURE_TYPE_EVENT_DESC,
            nullptr,
            0,
            ZE_EVENT_SCOPE_FLAG_DEVICE,
            ZE_EVENT_SCOPE_FLAG_HOST};

        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    for (auto i = 0u; i < 5; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        if (arguments.useEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        }
    }
    const size_t randomAccessBytesPerThread = accessMode == AccessMode::ReadWrite ? srcBufferAccessElementSize * 2 : srcBufferAccessElementSize;
    const size_t bytesTransferred = workItemCnt * (randomAccessBytesPerThread + offsetAccessBytesPerThread);

    // Benchmark
    for (auto i = 0u; i < iterations; i++) {
        if (!arguments.useEvents) {
            timer.measureStart();
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        if (arguments.useEvents) {

            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = levelzero.getAbsoluteKernelExecutionTime(timestampResult.global);
            statistics.pushValue(commandTime, bytesTransferred, typeSelector.getUnit(), typeSelector.getType());

            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            statistics.pushValue(timer.get(), bytesTransferred, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, offsetBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, result));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<RandomAccess> registerTestCase(run, Api::L0);
