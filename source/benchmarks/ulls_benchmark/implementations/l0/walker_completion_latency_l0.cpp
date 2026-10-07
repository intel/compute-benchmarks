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

#include "definitions/walker_completion_latency.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

static TestResult run(const WalkerCompletionLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    constexpr static auto bufferSize = 4096u;
    Timer timer;

    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC, nullptr, ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED};
    void *buffer = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize));
    volatile uint64_t *volatileBuffer = static_cast<uint64_t *>(buffer);

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_write_one.cl", "write_one_uncached", &kernel, &module, nullptr, ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY);
        result != TestResult::Success) {
        return result;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

    const ze_group_count_t groupCount{1, 1, 1};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    const ze_fence_desc_t fenceDesc{ZE_STRUCTURE_TYPE_FENCE_DESC, nullptr, 0};
    ze_fence_handle_t fence = nullptr;
    if (arguments.useFence) {
        ASSERT_ZE_RESULT_SUCCESS(zeFenceCreate(levelzero.commandQueue, &fenceDesc, &fence));
    }

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.useFence) {
            ASSERT_ZE_RESULT_SUCCESS(zeFenceReset(fence));
        }

        *volatileBuffer = 0;
        _mm_clflush(buffer);

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, arguments.useFence ? fence : nullptr));
        while (*volatileBuffer != 1) {
        }

        timer.measureStart();
        if (arguments.useFence) {
            ASSERT_ZE_RESULT_SUCCESS(zeFenceHostSynchronize(fence, std::numeric_limits<uint64_t>::max()));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    if (arguments.useFence) {
        ASSERT_ZE_RESULT_SUCCESS(zeFenceDestroy(fence));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<WalkerCompletionLatency> registerTestCase(run, Api::L0);
