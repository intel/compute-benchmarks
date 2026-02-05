/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/in_order_wait_append.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>

static TestResult run(const InOrderWaitAppendArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    if (arguments.counterBasedEvents && !levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    Timer timer;

    ze_command_list_handle_t commandList = nullptr;
    ze_command_queue_desc_t commandQueueDesc = zeDefaultGPUImmediateCommandQueueDesc;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &commandList));

    // create event
    ze_event_handle_t event = nullptr;
    ze_event_pool_handle_t eventPool = nullptr;
    if (!arguments.counterBasedEvents) {
        ze_event_pool_desc_t eventPoolDesc{
            .stype = ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
            .flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
            .count = 1u};
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        ze_event_desc_t eventDesc{
            .stype = ZE_STRUCTURE_TYPE_EVENT_DESC,
            .index = 0};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    } else {
        ze_event_counter_based_desc_t eventDescCBE = defaultIntelCounterBasedEventDesc;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDescCBE, &event));
    }

    // kernel for delaying the signal of event
    ze_module_handle_t module = nullptr;
    ze_kernel_handle_t kernel = nullptr;
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernel, &module, nullptr);
    if (kernelLoadRes != TestResult::Success)
        return kernelLoadRes;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.isCompleted ? 0 : arguments.kernelExecutionTime * 4);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));

    const ze_group_count_t groupCount{1, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(commandList, kernel, &groupCount, event, 0, nullptr));
    if (arguments.isCompleted) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(commandList, std::numeric_limits<uint64_t>::max()));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(commandList, 1, &event));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(commandList, std::numeric_limits<uint64_t>::max()));

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    if (eventPool != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<InOrderWaitAppend> registerTestCase(run, Api::L0);
