/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/completion_latency.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

static TestResult run(const CompletionLatencyArguments &arguments, Statistics &statistics) {
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

    // Create command list writing 1 to the buffer
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, static_cast<uint64_t *>(buffer), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *volatileBuffer = timestampInitial;
        _mm_clflush(buffer);

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        while (*volatileBuffer == timestampInitial) {
        }
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CompletionLatency> registerTestCase(run, Api::L0);
