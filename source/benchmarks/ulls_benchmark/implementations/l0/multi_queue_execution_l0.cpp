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

#include "definitions/multi_queue_execution.h"

#include <gtest/gtest.h>

static TestResult run(const MultiQueueExecutionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(true);
    LevelZero levelzero(extensionProperties);

    const size_t lws = 32u;
    const size_t gws = 32u;

    // Create kernel
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_multi_kernel_execution.cl", "empty", &kernel, &module);
    if (kernelLoadRes != TestResult::Success)
        return kernelLoadRes;

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));

    // Create command lists
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.useIoq) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    commandQueueDesc.priority = QueueProperties::create().setPriority(arguments.measuredQueuePriority).priority;
    ze_command_list_handle_t mainCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &mainCmdList));

    commandQueueDesc.priority = QueueProperties::create().setPriority(arguments.secondaryQueuePriority).priority;
    ze_command_list_handle_t secondCmdList{};
    if (arguments.useSecondaryQueue) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &secondCmdList));
    } else {
        secondCmdList = mainCmdList;
    }

    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, static_cast<uint32_t>(arguments.kernelCount)};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));
    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
    counterBasedEventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
    counterBasedEventDesc.signalScope |= ZE_EVENT_SCOPE_FLAG_HOST;

    ze_event_handle_t waitEvent{};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &waitEvent));

    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};

    // Warmup
    for (auto j = 0u; j < arguments.kernelCount; j++) {
        auto dependentEvent = (!arguments.useIoq || j == 0) ? waitEvent : nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mainCmdList, kernel, &groupCount, nullptr, dependentEvent ? 1 : 0, &dependentEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(secondCmdList, kernel, &groupCount, nullptr, dependentEvent ? 1 : 0, &dependentEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(mainCmdList, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(secondCmdList, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(waitEvent));

    Timer timer{};
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        for (auto j = 0u; j < arguments.kernelCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mainCmdList, kernel, &groupCount, nullptr, 1, &waitEvent));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(secondCmdList, kernel, &groupCount, nullptr, 1, &waitEvent));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(mainCmdList, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(secondCmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(waitEvent));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mainCmdList));
    if (arguments.useSecondaryQueue) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(secondCmdList));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiQueueExecution> registerTestCase(run, Api::L0);
