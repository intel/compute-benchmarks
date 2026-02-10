/*
 * Copyright (C) 2022-2026 Intel Corporation
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

#include "definitions/multi_argument_kernel_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const MultiArgumentKernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    const std::string kernelName = "kernelWith" + std::to_string(arguments.argumentCount);
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_multi_argument_kernel_switch_latency.cl", kernelName, &kernel, &module, nullptr);
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(arguments.workgroupSize), 1u, 1u));

    std::vector<void *> allocations(arguments.argumentCount);
    for (auto allocationId = 0u; allocationId < arguments.argumentCount; ++allocationId) {
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, 4096u, &allocations[allocationId]));
    }

    const ze_group_count_t dispatchTraits{static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u};

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

    for (auto argId = 0u; argId < arguments.argumentCount; ++argId) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, argId, sizeof(void *), &allocations[argId]));
    }
    for (auto kernelId = 0u; kernelId < arguments.kernelCount; ++kernelId) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    }

    if (arguments.profiling) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
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

    // Cleanup
    if (arguments.profiling) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestampBuffer));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    for (auto i = 0u; i < arguments.argumentCount; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, allocations[i]));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiArgumentKernelSwitchLatency> registerTestCase(run, Api::L0);
