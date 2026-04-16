/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/unique_kernel_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const UniqueKernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (arguments.kernelCount < 2) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
    Timer timer;

    std::vector<ze_module_handle_t> modules(arguments.kernelCount);
    std::vector<ze_kernel_handle_t> kernels(arguments.kernelCount);

    for (auto i = 0u; i < arguments.kernelCount; ++i) {
        auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_unique_kernel_switch_latency.cl", "uniqueKernel", &kernels[i], &modules[i], nullptr);
        if (kernelLoadRes != TestResult::Success) {
            for (auto j = 0u; j < i; ++j) {
                EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[j]));
                EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(modules[j]));
            }
            return kernelLoadRes;
        }
    }

    for (auto &kernel : kernels) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(arguments.workgroupSize), 1u, 1u));
    }

    void *allocation = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, 4096u, &allocation));

    const ze_group_count_t dispatchTraits{static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u};

    for (auto &kernel : kernels) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(void *), &allocation));
    }

    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    const auto timestampBufferSize = sizeof(uint64_t) * 2;
    void *timestampBuffer = nullptr;
    uint64_t *beginTimestamp = nullptr;
    uint64_t *endTimestamp = nullptr;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    if (arguments.profiling) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, timestampBufferSize, 0, &timestampBuffer));
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, timestampBuffer, timestampBufferSize))
        beginTimestamp = static_cast<uint64_t *>(timestampBuffer);
        endTimestamp = beginTimestamp + 1;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
    }

    for (auto &kernel : kernels) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    }

    if (arguments.profiling) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    for (auto iteration = 0u; iteration < arguments.iterations; ++iteration) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.profiling) {
            auto totalTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
            totalTime *= timerResolution;
            statistics.pushValue(totalTime, typeSelector.getUnit(), MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    if (arguments.profiling) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestampBuffer));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, allocation));

    for (auto i = 0u; i < arguments.kernelCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(modules[i]));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UniqueKernelSwitchLatency> registerTestCase(run, Api::L0);
