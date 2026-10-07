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

#include "definitions/execute_command_list_with_indirect_access.h"

#include <gtest/gtest.h>

typedef struct _st_container st_container;

struct _st_container {
    int32_t *value;
    st_container *next;
};

static TestResult run(const ExecuteCommandListWithIndirectAccessArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create kernel
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_indirect_access_kernel.cl", "indirectAccess", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetIndirectAccess(kernel, ZE_KERNEL_INDIRECT_ACCESS_FLAG_HOST | ZE_KERNEL_INDIRECT_ACCESS_FLAG_DEVICE | ZE_KERNEL_INDIRECT_ACCESS_FLAG_SHARED));
    // create indirect allocations

    std::vector<int32_t *> indirectAllocations;

    for (uint32_t i = 0; i < arguments.IndirectAllocationsAmount; i++) {
        const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
        void *ptr = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, sizeof(int32_t), 4u, &ptr));
        indirectAllocations.push_back(static_cast<int32_t *>(ptr));
    }

    std::vector<st_container *> wrappedIndirectAllocations;
    st_container *lastContainer = nullptr;
    for (auto &allocation : indirectAllocations) {
        *allocation = 0;
        const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
        void *ptr = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, sizeof(st_container), 4u, &ptr));
        st_container *wrappedIndirectAllocation = static_cast<st_container *>(ptr);
        wrappedIndirectAllocation->next = lastContainer;
        wrappedIndirectAllocation->value = allocation;
        wrappedIndirectAllocations.push_back(wrappedIndirectAllocation);
        lastContainer = wrappedIndirectAllocation;
    }

    // Create command list
    const ze_group_count_t dispatchTraits{1u, 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(st_container *), &wrappedIndirectAllocations.back()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        void *temporaryPtr = nullptr;

        if (arguments.AllocateMemory) {
            const ze_device_mem_alloc_desc_t deviceAllocationDescriptor{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
            ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDescriptor, 4u, 4u, levelzero.device, &temporaryPtr));
            ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, temporaryPtr, 4u));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        EXPECT_EQ(arguments.IndirectAllocationsAmount, *wrappedIndirectAllocations.at(0)->value);
        *wrappedIndirectAllocations.at(0)->value = 0;

        if (arguments.AllocateMemory) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, temporaryPtr));
        }
    }

    // Cleanup
    for (uint32_t i = 0; i < arguments.IndirectAllocationsAmount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, wrappedIndirectAllocations[i]->value));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, wrappedIndirectAllocations[i]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListWithIndirectAccess> registerTestCase(run, Api::L0);
