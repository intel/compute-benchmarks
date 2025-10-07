/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/measurement_type.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/error.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"

#include "definitions/random_access_multi_resource.h"
#include "level_zero/ze_api.h"

#include <gtest/gtest.h>
#include <iostream>
#include <random>

static TestResult run(const RandomAccessMultiResourceArguments &arguments, Statistics &statistics) {
    const size_t iterations = arguments.iterations;
    const size_t firstSize = arguments.firstSize;
    const size_t secondSize = arguments.secondSize;
    const MeasurementType measurementType = MeasurementType::Gpu;

    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, measurementType);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(true);
    LevelZero levelzero(extensionProperties);

    ze_device_compute_properties_t computeProperties{ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetComputeProperties(levelzero.device, &computeProperties));

    const uint32_t workItemCnt = 10 * MemoryConstants::megaByte;
    const size_t srcBufferAccessElementSize = sizeof(uint32_t);
    const size_t offsetAccessBytesPerThread = sizeof(uint32_t);

    ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    if (deviceProperties.maxMemAllocSize < firstSize || deviceProperties.maxMemAllocSize < secondSize) {
        return TestResult::DeviceNotCapable;
    }

    ze_device_memory_properties_t memProperties{ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES};
    uint32_t memPropertiesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryProperties(levelzero.device, &memPropertiesCount, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryProperties(levelzero.device, &memPropertiesCount, &memProperties));

    // Consider the 3 allocations used in the benchmark and additional size due to alignment requirements
    const uint64_t maxMemoryRequiredByBenchmark = firstSize + secondSize + workItemCnt * offsetAccessBytesPerThread + 1;
    if (memPropertiesCount == 0 || memProperties.totalSize < maxMemoryRequiredByBenchmark) {
        return TestResult::DeviceNotCapable;
    }

    if ((firstSize / srcBufferAccessElementSize) > pow(2, offsetAccessBytesPerThread * 8u) ||
        (secondSize / srcBufferAccessElementSize) > pow(2, offsetAccessBytesPerThread * 8u)) {
        return TestResult::InvalidArgs;
    }

    // Create buffers
    void *fstSrcBuffer{}, *sndSrcBuffer{}, *offsetBuffer{}, *result{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.firstPlacement, levelzero, firstSize, &fstSrcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.secondPlacement, levelzero, secondSize, &sndSrcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Device, levelzero, 1, &result));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Shared, levelzero, workItemCnt * offsetAccessBytesPerThread, &offsetBuffer));

    const uint32_t fstMaxPossibleAccessIndex = static_cast<uint32_t>((firstSize / srcBufferAccessElementSize) - 1);
    const uint32_t sndMaxPossibleAccessIndex = static_cast<uint32_t>((secondSize / srcBufferAccessElementSize) - 1);
    auto maxPossibleAccessIndex = std::max(fstMaxPossibleAccessIndex, sndMaxPossibleAccessIndex);
    // Prepare offset bufffer
    uint32_t *randBuff = reinterpret_cast<uint32_t *>(offsetBuffer);
    const auto rngSeed = 0xBEEFu;
    std::mt19937 generator(rngSeed);
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
        "access_multiple_resources_random"};
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure dispatch parameters
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, computeProperties.maxGroupSizeX, 1, 1));
    const ze_group_count_t dispatchTraits{workItemCnt / computeProperties.maxGroupSizeX, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(fstSrcBuffer), &fstSrcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(sndSrcBuffer), &sndSrcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(randBuff), &randBuff));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, sizeof(result), &result));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 4, sizeof(fstMaxPossibleAccessIndex), &fstMaxPossibleAccessIndex));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 5, sizeof(sndMaxPossibleAccessIndex), &sndMaxPossibleAccessIndex));

    // Create command list
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &defaultCommandQueueDesc, &cmdList));

    // Create event for gpu time measurement
    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
    ze_event_handle_t event{};
    ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &event));

    // Warmup
    for (auto i = 0u; i < 5; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
    }
    const size_t bytesTransferred = workItemCnt * (srcBufferAccessElementSize * 2 + offsetAccessBytesPerThread);

    // Benchmark
    for (auto i = 0u; i < iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        ze_kernel_timestamp_result_t timestampResult{};
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
        auto commandTime = levelzero.getAbsoluteKernelExecutionTime(timestampResult.global);
        statistics.pushValue(commandTime, bytesTransferred, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, fstSrcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, sndSrcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, offsetBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, result));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<RandomAccessMultiResource> registerTestCase(run, Api::L0);
