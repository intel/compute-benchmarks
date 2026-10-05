/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/new_resources_with_gpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const NewResourcesWithGpuAccessArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    // Create kernel
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_fill_with_ones.cl", "fill_with_ones", &kernel, &module, nullptr, ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY);
        result != TestResult::Success) {
        return result;
    }

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

    const auto bufferSize = arguments.size;
    const ze_device_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    const ze_group_count_t dispatchTraits{groupsCount, 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    void *buffer = nullptr;

    // Keep the previous allocation alive while creating the next one so the driver hands out a fresh VA
    void *previousBuffer = nullptr;

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
        if (previousBuffer != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, previousBuffer));
        }
        previousBuffer = buffer;

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    if (previousBuffer != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, previousBuffer));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<NewResourcesWithGpuAccess> registerTestCase(run, Api::L0);
