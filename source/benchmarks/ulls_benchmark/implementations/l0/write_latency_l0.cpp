/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/write_latency.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

#define ADD_ENTER_SUPPORT 0

#if ADD_ENTER_SUPPORT
#include <iostream>
#endif

static TestResult run(const WriteLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    constexpr static auto bufferSize = 4096u;
    constexpr uint64_t timestampInitial = 0xffffffffu;
    Timer timer;

    // Create buffer
    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize));
    volatile uint64_t *volatileBuffer = static_cast<uint64_t *>(buffer);

    // Create event pool
    ze_event_pool_desc_t eventPoolDesc = {
        ZE_STRUCTURE_TYPE_EVENT_POOL_DESC,
        nullptr,
        ZE_EVENT_POOL_FLAG_HOST_VISIBLE,
        2};
    ze_event_pool_handle_t hEventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &hEventPool));

    // Create events
    const ze_event_desc_t eventDesc = {
        ZE_STRUCTURE_TYPE_EVENT_DESC,
        nullptr,
        0,
        ZE_EVENT_SCOPE_FLAG_DEVICE,
        ZE_EVENT_SCOPE_FLAG_HOST};
    ze_event_handle_t hEvent;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &hEvent));
    const ze_event_desc_t eventDesc2 = {
        ZE_STRUCTURE_TYPE_EVENT_DESC,
        nullptr,
        1,
        ZE_EVENT_SCOPE_FLAG_HOST,
        0u};
    ze_event_handle_t hEvent2;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc2, &hEvent2));

    // Create command list writing 1 to the buffer
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(cmdList, hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(cmdList, 1u, &hEvent2));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, static_cast<uint64_t *>(buffer), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendEventReset(cmdList, hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *volatileBuffer = timestampInitial;
        _mm_clflush(buffer);

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, std::numeric_limits<uint64_t>::max()));

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(hEvent2));
        while (*volatileBuffer == timestampInitial) {
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(hEvent2));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent2));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<WriteLatency> registerTestCase(run, Api::L0);
