/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/in_order_wait_append.h"

#include <gtest/gtest.h>

static TestResult run(const InOrderWaitAppendArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    if (arguments.counterBasedEvents) {
        extensionProperties.setCounterBasedCreateFunctions(true);
    }
    LevelZero levelzero{extensionProperties};
    Timer timer;

    // Create in-order immediate command list
    ze_command_queue_desc_t commandQueueDesc = {ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t commandList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &commandList));

    ze_event_handle_t event{};
    ze_event_pool_handle_t eventPool{};

    // Create event (either regular or counter-based)
    if (!arguments.counterBasedEvents) {
        ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, 1u};
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    } else {
        zex_counter_based_event_desc_t eventDescCBE{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
        eventDescCBE.stype = ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC;
        eventDescCBE.pNext = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &eventDescCBE, &event));
    }

    // Create dummy buffers for memcpy operations (to have actual work)
    constexpr size_t bufferSize = 64;
    void *srcBuffer{}, *dstBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, bufferSize, &srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, bufferSize, &dstBuffer));

    // Benchmark
    if (!arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(commandList, dstBuffer, srcBuffer, bufferSize, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max())); //wait untile the event is signaled

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWaitOnEvents(commandList, 1, &event)); //wait on signaled event, should be immediate
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, dstBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    if (eventPool) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(commandList));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<InOrderWaitAppend> registerTestCase(run, Api::L0);