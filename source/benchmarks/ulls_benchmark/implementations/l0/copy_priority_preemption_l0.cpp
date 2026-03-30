/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_priority_preemption.h"

#include <gtest/gtest.h>

static TestResult run(const CopyPriorityPreemptionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    // Allocate memory for long copy (normal priority)
    void *longSrc{};
    void *longDst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.src, levelzero, arguments.longCopySize, &longSrc));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dst, levelzero, arguments.longCopySize, &longDst));

    // Allocate memory for short copy (high priority)
    void *shortSrc{};
    void *shortDst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.src, levelzero, arguments.shortCopySize, &shortSrc));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dst, levelzero, arguments.shortCopySize, &shortDst));

    // Create immediate in-order command list with normal priority
    ze_command_queue_desc_t normalQueueDesc{.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC, .pNext = nullptr, .ordinal = levelzero.commandQueueDesc.ordinal, .index = 0, .flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER, .mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, .priority = ZE_COMMAND_QUEUE_PRIORITY_NORMAL};
    ze_command_list_handle_t normalCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &normalQueueDesc, &normalCmdList));

    // Create immediate in-order command list with high priority
    ze_command_queue_desc_t highQueueDesc{.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC, .pNext = nullptr, .ordinal = levelzero.commandQueueDesc.ordinal, .index = 0, .flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER, .mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS, .priority = ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH};
    ze_command_list_handle_t highCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &highQueueDesc, &highCmdList));

    // Create counter-based events for profiling both copies
    ze_event_counter_based_flags_t cbFlags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE | ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP | ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE;
    ze_event_counter_based_desc_t cbDesc{.stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC, .pNext = nullptr, .flags = cbFlags, .signal = ZE_EVENT_SCOPE_FLAG_HOST, .wait = 0};

    ze_event_handle_t npEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &cbDesc, &npEvent));

    ze_event_handle_t hpEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &cbDesc, &hpEvent));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        // Submit long copy on normal-priority queue
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(normalCmdList, longDst, longSrc, arguments.longCopySize, npEvent, 0, nullptr));

        // Submit short copy on high-priority queue
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(highCmdList, shortDst, shortSrc, arguments.shortCopySize, hpEvent, 0, nullptr));

        // Wait for high-priority copy to complete
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(highCmdList, std::numeric_limits<uint64_t>::max()));

        // Measure time from NP.start to HP.end (lower is better)
        ze_kernel_timestamp_result_t npTimestamp{};
        ze_kernel_timestamp_result_t hpTimestamp{};
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(npEvent, &npTimestamp));
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(hpEvent, &hpTimestamp));

        auto gpuTime = std::chrono::nanoseconds((hpTimestamp.global.kernelEnd - npTimestamp.global.kernelStart) * timerResolution);
        statistics.pushValue(gpuTime, typeSelector.getUnit(), typeSelector.getType());

        // Wait for normal-priority copy before next iteration
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(normalCmdList, std::numeric_limits<uint64_t>::max()));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(npEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hpEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(normalCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(highCmdList));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.src, levelzero, longSrc));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.dst, levelzero, longDst));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.src, levelzero, shortSrc));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.dst, levelzero, shortDst));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopyPriorityPreemption> registerTestCase(run, Api::L0);
