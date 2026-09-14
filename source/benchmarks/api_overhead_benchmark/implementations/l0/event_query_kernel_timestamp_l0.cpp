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

#include "definitions/event_query_kernel_timestamp.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>
#include <limits>

static TestResult run(const EventQueryKernelTimestampArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    Timer timer;

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    auto kernelLoadResult = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_empty_kernel.cl", "empty", &kernel, &module, nullptr);
    if (kernelLoadResult != TestResult::Success) {
        return kernelLoadResult;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    const ze_group_count_t dispatchTraits{1u, 1u, 1u};

    ze_command_list_handle_t cmdList{};
    if (arguments.useImmediate) {
        ze_command_queue_desc_t commandQueueDesc = zeDefaultGPUImmediateCommandQueueDesc;
        commandQueueDesc.ordinal = levelzero.commandQueueDesc.ordinal;
        commandQueueDesc.index = levelzero.commandQueueDesc.index;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));
    } else {
        ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
        cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
        cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER; // counter based events are rejected on an out of order list
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    }

    ze_event_counter_based_desc_t eventDesc = defaultIntelCounterBasedEventDesc;
    eventDesc.flags = ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE | ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP |
                      (arguments.useImmediate ? ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE : ZE_EVENT_COUNTER_BASED_FLAG_NON_IMMEDIATE);

    ze_kernel_timestamp_result_t timestamp{};
    bool timestampPopulated = true;

    for (size_t i = 0u; i < arguments.iterations; i++) {
        ze_event_handle_t event{};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &event));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
        if (!arguments.useImmediate) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        timer.measureStart();
        auto status = zeEventQueryKernelTimestamp(event, &timestamp);
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(status);
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType(), "First");
        timestampPopulated = timestampPopulated && (timestamp.global.kernelEnd != 0u);

        timer.measureStart();
        for (size_t j = 0u; j < arguments.queryCount; j++) {
            status = zeEventQueryKernelTimestamp(event, &timestamp);
            if (status != ZE_RESULT_SUCCESS) {
                break;
            }
        }
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(status);
        statistics.pushValue(timer.get() / arguments.queryCount, typeSelector.getUnit(), typeSelector.getType(), "Repeated");

        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        if (!arguments.useImmediate) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return timestampPopulated ? TestResult::Success : TestResult::VerificationFail;
}

static RegisterTestCaseImplementation<EventQueryKernelTimestamp> registerTestCase(run, Api::L0);
