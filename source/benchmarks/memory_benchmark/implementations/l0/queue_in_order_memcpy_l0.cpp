/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/queue_in_order_memcpy.h"

#include <gtest/gtest.h>

static TestResult run(const QueueInOrderMemcpyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.isCopyOnly).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
        (arguments.sourcePlacement == UsmMemoryPlacement::NonUsmImported ||
         arguments.destinationPlacement == UsmMemoryPlacement::NonUsmImported));

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    // Create buffers
    void *source{}, *destination{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &destination));

    // Create events
    ze_event_pool_handle_t eventPool{};
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = static_cast<uint32_t>(arguments.count);
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    std::vector<ze_event_handle_t> events;
    for (auto j = 0u; j < arguments.count; ++j) {
        ze_event_handle_t event{};
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        eventDesc.index = j;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
        events.push_back(event);
    }

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &levelzero.commandQueueDesc, &cmdList));
    // Warmup
    ze_event_handle_t signalEvent = events[0];
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, signalEvent, 0, nullptr));
    ze_event_handle_t waitEvent = signalEvent;
    for (auto j = 1u; j < arguments.count; ++j) {
        signalEvent = events[j];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, signalEvent, 1, &waitEvent));
        waitEvent = signalEvent;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(waitEvent, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        for (auto &event : events) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }
        timer.measureStart();
        signalEvent = events[0];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, signalEvent, 0, nullptr));
        waitEvent = signalEvent;
        for (auto j = 1u; j < arguments.count; ++j) {
            signalEvent = events[j];
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, signalEvent, 1, &waitEvent));
            waitEvent = signalEvent;
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(waitEvent, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (auto &event : events) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, destination));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<QueueInOrderMemcpy> registerTestCase(run, Api::L0);
