/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/event_reuse_imm_cmdlist_in_progress.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const EventReuseImmCmdlistInProgressArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    Timer timer;

    ze_event_handle_t event = nullptr;
    const ze_event_counter_based_desc_t counterBasedEventDesc{
        .stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC,
        .flags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE | ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &counterBasedEventDesc, &event));

    ze_command_list_handle_t commandList = nullptr;
    const ze_command_queue_desc_t commandQueueDesc{
        .stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,
        .ordinal = levelzero.commandQueueDesc.ordinal,
        .index = levelzero.commandQueueDesc.index,
        .flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
        .mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &commandList));

    ze_module_handle_t emptyModule = nullptr;
    ze_kernel_handle_t emptyKernel = nullptr;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_empty_kernel.cl", "empty", &emptyKernel, &emptyModule, nullptr);
        result != TestResult::Success) {
        return result;
    }

    ze_module_handle_t eatTimeModule = nullptr;
    ze_kernel_handle_t eatTimeKernel = nullptr;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &eatTimeKernel, &eatTimeModule, nullptr);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(eatTimeKernel, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(emptyKernel, 1u, 1u, 1u));
    constexpr size_t approximateEatTimeOperationsPerMicrosecond = 4;
    const int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime * approximateEatTimeOperationsPerMicrosecond);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(eatTimeKernel, 0, sizeof(int), &kernelOperationsCount));

    const ze_group_count_t groupCount{1, 1, 1};

    for (size_t i = 0; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandList, eatTimeKernel, &groupCount, event, 0u, nullptr));
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandList, emptyKernel, &groupCount, event, 0u, nullptr));
        timer.measureEnd();

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(commandList, UINT64_MAX));

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event));

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(eatTimeKernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(eatTimeModule));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(emptyKernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(emptyModule));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandList));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<EventReuseImmCmdlistInProgress> registerTestCase(run, Api::L0);
