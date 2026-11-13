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
#include "framework/utility/timer.h"

#include "definitions/round_trip_ping_pong.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const RoundTripPingPongArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;
    bool copiesThroughHost = false;
    bool accessesFromCpu = false;
    bool usesSeparateHostAllocation = true;
    switch (arguments.placement) {
    case UsmRuntimeMemoryPlacement::Device:
        copiesThroughHost = true;
        break;
    case UsmRuntimeMemoryPlacement::Shared:
        accessesFromCpu = true;
        break;
    case UsmRuntimeMemoryPlacement::Host:
        usesSeparateHostAllocation = false;
        break;
    default:
        return TestResult::InvalidArgs;
    }

    void *kernelValue = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.placement, levelzero, sizeof(int), &kernelValue));
    void *hostAllocation = kernelValue;
    if (usesSeparateHostAllocation) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Host, levelzero, sizeof(int), &hostAllocation));
    }
    int *const hostValue = static_cast<int *>(hostAllocation);
    int *const sharedValue = static_cast<int *>(kernelValue);
    *hostValue = 0;

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_increment.cl", "increment", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(kernelValue), &kernelValue));

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    const ze_group_count_t groupCount{1u, 1u, 1u};
    if (copiesThroughHost) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, kernelValue, hostValue, sizeof(int), nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr));
    }
    if (accessesFromCpu) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemAdvise(cmdList, levelzero.device, kernelValue, sizeof(int), ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    if (copiesThroughHost) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, hostValue, kernelValue, sizeof(int), nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        if (accessesFromCpu) {
            *sharedValue = *hostValue;
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        if (accessesFromCpu) {
            *hostValue = *sharedValue;
        }
        (*hostValue)--;
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    const bool isValueRestored = *hostValue == 0;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    if (usesSeparateHostAllocation) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostAllocation));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, kernelValue));
    return isValueRestored ? TestResult::Success : TestResult::VerificationFail;
}

static RegisterTestCaseImplementation<RoundTripPingPong> registerTestCase(run, Api::L0);
