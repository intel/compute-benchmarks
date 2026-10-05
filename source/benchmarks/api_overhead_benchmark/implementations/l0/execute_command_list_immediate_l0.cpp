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

#include "definitions/execute_command_list_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    LevelZero levelzero(extensionProperties);
    Timer timer;

    // Create kernel
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useIoq) {
        ze_event_counter_based_flags_t counterBasedDescFlags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE | ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE;
        if (arguments.useProfiling) {
            counterBasedDescFlags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
        }
        const ze_event_scope_flags_t signalScope = ZE_EVENT_SCOPE_FLAG_HOST;
        const ze_event_scope_flags_t waitScope = 0;
        ze_event_counter_based_desc_t cbDesc{.stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC, .pNext = nullptr, .flags = counterBasedDescFlags, .signal = signalScope, .wait = waitScope};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &cbDesc, &event));
    } else {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        if (arguments.useProfiling) {
            eventPoolDesc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
        }

        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime * 4);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));

    // Create an immediate command list
    const ze_group_count_t groupCount{1, 1, 1};
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.useIoq) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        auto limit = arguments.useBarrierSynchronization ? arguments.amountOfCalls : arguments.amountOfCalls - 1;

        for (uint32_t callId = 0u; callId < limit; callId++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }
        // last call synchronizes
        if (!arguments.useBarrierSynchronization) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, arguments.useEventForHostSync ? event : nullptr, 0, nullptr));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, arguments.useEventForHostSync ? event : nullptr, 0u, nullptr));
        }

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        if (arguments.useEventForHostSync) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        }
        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
        if (arguments.useEventForHostSync) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryStatus(event));
            if (!arguments.useIoq) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
            }
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    if (!arguments.useIoq) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListImmediate> registerTestCase(run, Api::L0);
