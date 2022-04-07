/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_fill.h"

#include <gtest/gtest.h>

static TestResult run(const UsmFillArguments &arguments, Statistics &statistics) {
    ContextProperties contextProperties = ContextProperties::create().create().setDeviceSelection(arguments.contextPlacement).allowCreationFail();
    QueueProperties queueProperties = QueueProperties::create().setDeviceSelection(arguments.queuePlacement).setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties, contextProperties);
    if (levelzero.context == nullptr || levelzero.commandQueue == nullptr || arguments.patternSize > levelzero.commandQueueMaxFillSize) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResoultion(arguments.queuePlacement);

    // Create buffer
    void *buffer{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.bufferPlacement, levelzero, arguments.size, &buffer));

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useEvents) {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.commandQueueDevice, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Make the buffer resident
    for (DeviceSelection device : DeviceSelectionHelper::split(arguments.contextPlacement)) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(device), buffer, arguments.size));
    }

    // Create pattern
    const auto pattern = std::make_unique<uint8_t[]>(arguments.patternSize);

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.commandQueueDevice, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, buffer, pattern.get(), arguments.patternSize, arguments.size, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
            commandTime *= timerResolution;
            statistics.pushValue(commandTime, arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
        }
    }

    // Evict buffer
    for (DeviceSelection device : DeviceSelectionHelper::split(arguments.contextPlacement)) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(device), buffer, arguments.size));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmFill> registerTestCase(run, Api::L0);
