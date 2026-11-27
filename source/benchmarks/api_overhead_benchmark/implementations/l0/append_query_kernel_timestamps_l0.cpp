/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/append_query_kernel_timestamps.h"

#include <vector>

using namespace L0;

static TestResult run(const AppendQueryKernelTimestampsArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    const uint32_t eventCount = static_cast<uint32_t>(arguments.eventCount);
    const ze_event_pool_desc_t timestampPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, eventCount};
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t timestampPool{};
    std::vector<ze_event_handle_t> timestampEvents(eventCount);
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &timestampPoolDesc, 0, nullptr, &timestampPool));
    for (auto k = 0u; k < eventCount; ++k) {
        eventDesc.index = k;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(timestampPool, &eventDesc, &timestampEvents[k]));
    }

    const size_t resultSize = sizeof(ze_kernel_timestamp_result_t);
    void *dst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dst, levelzero, eventCount * resultSize, &dst));
    void *offsetsBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, eventCount * sizeof(size_t), &offsetsBuffer));
    auto *const offsets = static_cast<size_t *>(offsetsBuffer);
    for (auto k = 0u; k < eventCount; ++k) {
        offsets[k] = k * resultSize;
    }
    const size_t *const pOffsets = arguments.useOffsets ? offsets : nullptr;

    const uint32_t appendCount = static_cast<uint32_t>(arguments.appendCount);
    const ze_event_pool_desc_t signalPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, appendCount};
    ze_event_pool_handle_t signalPool{};
    std::vector<ze_event_handle_t> signalEvents(appendCount);
    if (arguments.useEvent) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &signalPoolDesc, 0, nullptr, &signalPool));
        for (auto j = 0u; j < appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(signalPool, &eventDesc, &signalEvents[j]));
        }
    }

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    for (auto i = 0u; i < arguments.iterations; ++i) {
        timer.measureStart();
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendQueryKernelTimestamps(cmdList, eventCount, timestampEvents.data(), dst, pOffsets, signalEvents[j], 0, nullptr));
        }
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        statistics.pushValue(timer.get() / appendCount, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvent) {
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(signalEvents[j]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(signalPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.dst, levelzero, dst));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Host, levelzero, offsetsBuffer));
    for (auto k = 0u; k < eventCount; ++k) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(timestampEvents[k]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(timestampPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendQueryKernelTimestamps> registerTestCase(run, Api::L0);
