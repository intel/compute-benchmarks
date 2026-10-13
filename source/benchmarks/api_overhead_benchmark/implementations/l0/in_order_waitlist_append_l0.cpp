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

#include "definitions/in_order_waitlist_append.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>

using namespace L0;

static TestResult run(const InOrderWaitlistAppendArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    if (arguments.counterBasedEvents && !levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    Timer timer;

    ze_command_list_handle_t commandList = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &commandList));

    ze_event_handle_t event = nullptr;
    ze_event_pool_handle_t eventPool = nullptr;
    if (arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &defaultIntelCounterBasedEventDesc, &event));
    } else {
        const ze_event_pool_desc_t eventPoolDesc{
            .stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
            .flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
            .count = 1u};
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        const ze_event_desc_t eventDesc{
            .stype = ZE_STRUCTURE_TYPE_EVENT_DESC,
            .index = 0,
            .signal = ZE_EVENT_SCOPE_FLAG_HOST,
            .wait = ZE_EVENT_SCOPE_FLAG_DEVICE};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    ze_module_handle_t signalModule = nullptr;
    ze_kernel_handle_t signalKernel = nullptr;
    if (const auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &signalKernel, &signalModule, nullptr);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(signalKernel, 1u, 1u, 1u));
    constexpr int approximateEatTimeOperationsPerMicrosecond = 4;
    const int kernelOperationsCount = static_cast<int>(arguments.isCompleted ? 0 : arguments.kernelExecutionTime * approximateEatTimeOperationsPerMicrosecond);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(signalKernel, 0, sizeof(int), &kernelOperationsCount));

    ze_module_handle_t measuredModule = nullptr;
    ze_kernel_handle_t measuredKernel = nullptr;
    if (const auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_empty_kernel.cl", "empty", &measuredKernel, &measuredModule, nullptr);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(measuredKernel, 1u, 1u, 1u));

    const ze_group_count_t groupCount{1, 1, 1};

    for (auto i = 0u; i < arguments.iterations; i++) {
        if (!arguments.counterBasedEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandList, signalKernel, &groupCount, event, 0, nullptr));
        if (arguments.isCompleted) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(commandList, std::numeric_limits<uint64_t>::max()));
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandList, measuredKernel, &groupCount, nullptr, 1u, &event));
        timer.measureEnd();

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(commandList, std::numeric_limits<uint64_t>::max()));

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    if (eventPool != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(measuredKernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(measuredModule));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(signalKernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(signalModule));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<InOrderWaitlistAppend> registerTestCase(run, Api::L0);
