/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/new_resources_with_gpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const NewResourcesWithGpuAccessArguments &arguments, Statistics &statistics) {
    LevelZero levelzero;
    Timer timer;

    // Create kernel
    auto kernelBinary = FileHelper::loadBinaryFile("ulls_benchmark_fill_with_ones.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module;
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "fill_with_ones";
    ze_kernel_handle_t kernel;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Get work size
    const uint32_t elements = static_cast<uint32_t>(arguments.size) / sizeof(uint32_t);
    uint32_t groupSizeX{}, groupSizeY{}, groupSizeZ{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(kernel, static_cast<uint32_t>(arguments.size), 1, 1, &groupSizeX, &groupSizeY, &groupSizeZ));
    uint32_t groupsCount = elements / groupSizeX; // may be rounded down, but that shouldn't matter for big buffers
    if (groupsCount == 0) {
        // very small buffer
        groupSizeX = elements;
        groupsCount = 1;
    }

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, 1, 1));

    // Create buffer for warmup
    const auto bufferSize = arguments.size;
    const ze_device_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &allocationDesc, bufferSize, 0, levelzero.device, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize));

    // Warmup
    const ze_group_count_t dispatchTraits{groupsCount, 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Cleanup after warmup
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        // Create buffer
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &allocationDesc, bufferSize, 0, levelzero.device, &buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize));

        // Create command list to write 1
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

        // Dispatch kernel and wait for completion
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        timer.measureEnd();

        // Cleanup after iteration
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<NewResourcesWithGpuAccess> registerTestCase(run, Api::L0);
