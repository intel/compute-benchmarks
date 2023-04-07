/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/random_access.h"

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
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    const uint32_t workGroupSize = 10 * MemoryConstants::megaByte;
    const uint32_t threadsPerWorkGroup = 1 * MemoryConstants::kiloByte;
    const size_t srcBufferAccessElementSize = sizeof(uint32_t);
    const size_t offsetAccessBytesPerThread = sizeof(uint32_t);

    ze_device_properties_t deviceProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    if (deviceProperties.maxMemAllocSize < arguments.allocationSize) {
        return TestResult::DeviceNotCapable;
    }

    ze_device_memory_properties_t memProperties{};
    memProperties.stype = ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES;
    memProperties.pNext = nullptr;
    uint32_t memPropertiesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryProperties(levelzero.device, &memPropertiesCount, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetMemoryProperties(levelzero.device, &memPropertiesCount, &memProperties));

    // Consider the 3 allocations used in the benchmark and additional size due to alignment requirements
    const uint64_t maxMemoryRequiredByBenchmark = arguments.allocationSize + workGroupSize * offsetAccessBytesPerThread + 1 +
                                                  arguments.alignment * 3u;
    if (memPropertiesCount == 0 || memProperties.totalSize < maxMemoryRequiredByBenchmark) {
        return TestResult::DeviceNotCapable;
    }

    if ((arguments.allocationSize / srcBufferAccessElementSize) > pow(2, offsetAccessBytesPerThread * 8u)) {
        return TestResult::InvalidArgs;
    }

    // Create buffer
    const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};

    void *srcBuffer{}, *offsetBuffer{}, *result;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, arguments.allocationSize, arguments.alignment, levelzero.device, &srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, 1, arguments.alignment, levelzero.device, &result));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(levelzero.context, &deviceAllocationDesc, &hostAllocationDesc, workGroupSize * offsetAccessBytesPerThread, arguments.alignment, levelzero.device, &offsetBuffer));

    const double maxPossibleAccessIndex = (arguments.allocationSize / srcBufferAccessElementSize) - 1;

    // Prepare Offset bufffer
    uint32_t *randBuff = reinterpret_cast<uint32_t *>(offsetBuffer);
    std::random_device randomDevice;
    std::mt19937 generator(randomDevice());
    std::uniform_int_distribution<uint32_t> distr(0, maxPossibleAccessIndex - 1);
    for (auto index = 0u; index < workGroupSize; index++) {
        randBuff[index] = distr(generator);
    }

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("access_device_mem_random.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "access_device_memory_random";
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure dispatch parameters
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, threadsPerWorkGroup, 1, 1));
    const ze_group_count_t dispatchTraits{workGroupSize / threadsPerWorkGroup, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(srcBuffer), &srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(randBuff), &randBuff));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(result), &result));

    const std::string accessModeArg = static_cast<const std::string &>(arguments.accessMode);
    const uint32_t accessMode = getAccessMode(accessModeArg);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, sizeof(accessMode), &accessMode));
    const uint32_t randomAccessRange = arguments.randomAccessRange;
    const uint32_t maxAccessIndex = std::min<uint32_t>(randomAccessRange, 100u) * maxPossibleAccessIndex / 100.0;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 4, sizeof(maxAccessIndex), &maxAccessIndex));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    for (auto i = 0u; i < 5; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    }
    const size_t randomAccessBytesPerThread = accessMode == AccessMode::ReadWrite ? srcBufferAccessElementSize * 2 : srcBufferAccessElementSize;
    const size_t bytesTransferred = workGroupSize * (randomAccessBytesPerThread + offsetAccessBytesPerThread);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), bytesTransferred, MeasurementUnit::GigabytesPerSecond, typeSelector.getType());
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
