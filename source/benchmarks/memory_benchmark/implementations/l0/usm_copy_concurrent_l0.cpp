/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_concurrent.h"

#include <gtest/gtest.h>

static TestResult run(const UsmConcurrentCopyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties;
    queueProperties.disable();
    LevelZero levelzero(queueProperties);

    auto h2dQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.h2dEngine);
    if (nullptr == h2dQueueDesc) {
        return TestResult::DeviceNotCapable;
    }

    auto d2hQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.d2hEngine);
    if (nullptr == d2hQueueDesc) {
        return TestResult::DeviceNotCapable;
    }

    zex_intel_queue_copy_operations_offload_hint_exp_desc_t copyOffload = {ZEX_INTEL_STRUCTURE_TYPE_QUEUE_COPY_OPERATIONS_OFFLOAD_HINT_EXP_PROPERTIES, nullptr, true};
    if (arguments.withCopyOffload) {
        h2dQueueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
        d2hQueueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;

        h2dQueueDesc->desc.pNext = &copyOffload;
        d2hQueueDesc->desc.pNext = &copyOffload;
    }

    // Create events
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 3u;
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    ze_event_handle_t h2dEvent;
    ze_event_handle_t d2hEvent;
    ze_event_handle_t waitEvent;

    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &h2dEvent));
    eventDesc.index = 1u;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &d2hEvent));
    eventDesc.index = 2u;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &waitEvent));

    ze_command_list_handle_t h2dCommandList;
    ze_command_list_handle_t d2hCommandList;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &h2dQueueDesc->desc, &h2dCommandList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &d2hQueueDesc->desc, &d2hCommandList));

    Timer timer;

    // Create buffers
    void *host1{}, *device1{}, *host2{}, *device2{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &host1));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &device1));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &host2));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &device2));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(h2dCommandList, device1, host1, arguments.size, h2dEvent, 1, &waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(d2hCommandList, host2, device2, arguments.size, d2hEvent, 1, &waitEvent));

    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(h2dEvent, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(d2hEvent, std::numeric_limits<uint64_t>::max()));

    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(h2dEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(d2hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(waitEvent));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(h2dCommandList, device1, host1, arguments.size, h2dEvent, 1, &waitEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(d2hCommandList, host2, device2, arguments.size, d2hEvent, 1, &waitEvent));
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(h2dEvent, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(d2hEvent, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.size * 2, typeSelector.getUnit(), typeSelector.getType());
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(h2dEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(d2hEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(waitEvent));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(h2dCommandList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(d2hCommandList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(h2dEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(d2hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, host1));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, host2));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, device1));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, device2));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmConcurrentCopy> registerTestCase(run, Api::L0);
