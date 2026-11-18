/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_concurrent.h"

using namespace L0;

static TestResult run(const UsmConcurrentCopyArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties;
    queueProperties.disable();
    LevelZero levelzero(queueProperties);

    const auto h2dQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.h2dEngine);
    if (nullptr == h2dQueueDesc) {
        return TestResult::DeviceNotCapable;
    }

    const auto d2hQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.d2hEngine);
    if (nullptr == d2hQueueDesc) {
        return TestResult::DeviceNotCapable;
    }

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

    ze_command_queue_handle_t h2dQueue;
    ze_command_queue_handle_t d2hQueue;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero.context, levelzero.device, &h2dQueueDesc->desc, &h2dQueue));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero.context, levelzero.device, &d2hQueueDesc->desc, &d2hQueue));

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    if (arguments.withCopyOffload) {
        cmdListDesc.flags |= ZE_COMMAND_LIST_FLAG_COPY_OFFLOAD_HINT;
    }

    ze_command_list_handle_t h2dCommandList;
    ze_command_list_handle_t d2hCommandList;
    cmdListDesc.commandQueueGroupOrdinal = h2dQueueDesc->desc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &h2dCommandList));
    cmdListDesc.commandQueueGroupOrdinal = d2hQueueDesc->desc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &d2hCommandList));

    Timer timer, h2dTimer, d2hTimer;

    void *host1{}, *device1{}, *host2{}, *device2{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &host1));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &device1));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &host2));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &device2));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(h2dCommandList, device1, host1, arguments.size, h2dEvent, 1, &waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(d2hCommandList, host2, device2, arguments.size, d2hEvent, 1, &waitEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(h2dCommandList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(d2hCommandList));

    for (auto i = 0u; i < arguments.iterations; i++) {
        auto h2dDone = false;
        auto d2hDone = false;

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(h2dQueue, 1, &h2dCommandList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(d2hQueue, 1, &d2hCommandList, nullptr));
        timer.measureStart();
        h2dTimer.measureStart();
        d2hTimer.measureStart();

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEvent));
        while (!h2dDone || !d2hDone) {
            if (!h2dDone) {
                const ze_result_t status = zeEventHostSynchronize(h2dEvent, 0u);
                if (status != ZE_RESULT_NOT_READY) {
                    ASSERT_ZE_RESULT_SUCCESS(status);
                    h2dTimer.measureEnd();
                    h2dDone = true;
                }
            }
            if (!d2hDone) {
                const ze_result_t status = zeEventHostSynchronize(d2hEvent, 0u);
                if (status != ZE_RESULT_NOT_READY) {
                    ASSERT_ZE_RESULT_SUCCESS(status);
                    d2hTimer.measureEnd();
                    d2hDone = true;
                }
            }
        }

        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.size * 2, typeSelector.getUnit(), typeSelector.getType(), "Total ");
        statistics.pushValue(h2dTimer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType(), "H2D ");
        statistics.pushValue(d2hTimer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType(), "D2H ");

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(h2dQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(d2hQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(h2dEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(d2hEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(waitEvent));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(h2dCommandList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(d2hCommandList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(h2dQueue));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(d2hQueue));
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
