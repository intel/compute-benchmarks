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

#include "definitions/usm_copy_concurrent_graph.h"

using namespace L0;

static TestResult run(const UsmConcurrentCopyGraphArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{QueueProperties::create().disable()};
    if (!levelzero.isExtensionSupported(ZE_RECORD_REPLAY_GRAPH_EXP_NAME)) {
        return TestResult::ApiNotCapable;
    }
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    levelzero.initializeExtension(ExtensionProperties::create().setGraphFunctions(true));
    const auto &graphApi = levelzero.graphExtension;

    const auto replayQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    if (replayQueueDesc == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    replayQueueDesc->desc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;

    ze_command_queue_desc_t captureQueueDesc = replayQueueDesc->desc;
    zex_intel_queue_copy_operations_offload_hint_exp_desc_t copyOffload = {ZEX_INTEL_STRUCTURE_TYPE_QUEUE_COPY_OPERATIONS_OFFLOAD_HINT_EXP_PROPERTIES, nullptr, true};
    if (arguments.withCopyOffload) {
        captureQueueDesc.pNext = &copyOffload;
    }

    ze_command_list_handle_t replayList{};
    ze_command_list_handle_t rootList{};
    ze_command_list_handle_t h2dList{};
    ze_command_list_handle_t d2hList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &replayQueueDesc->desc, &replayList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &captureQueueDesc, &rootList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &captureQueueDesc, &h2dList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &captureQueueDesc, &d2hList));

    void *host1{}, *device1{}, *host2{}, *device2{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &host1));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &device1));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &host2));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &device2));

    constexpr static auto timestampCount = 3u;
    const ze_host_mem_alloc_desc_t hostAllocDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    uint64_t *timestamps{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocDesc, timestampCount * sizeof(uint64_t), 0, reinterpret_cast<void **>(&timestamps)));
    uint64_t *const startTimestamp = &timestamps[0];
    uint64_t *const h2dEndTimestamp = &timestamps[1];
    uint64_t *const d2hEndTimestamp = &timestamps[2];

    const ze_event_counter_based_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC, nullptr, ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE,
                                                  ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_handle_t forkEvent{};
    ze_event_handle_t joinEvents[2]{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &forkEvent));
    for (auto &joinEvent : joinEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &joinEvent));
    }

    ze_graph_handle_t graph{};
    ASSERT_ZE_RESULT_SUCCESS(graphApi.graphCreate(levelzero.context, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListBeginCaptureIntoGraph(rootList, graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(rootList, startTimestamp, forkEvent, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(h2dList, device1, host1, arguments.size, nullptr, 1, &forkEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(h2dList, h2dEndTimestamp, joinEvents[0], 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(d2hList, host2, device2, arguments.size, nullptr, 1, &forkEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(d2hList, d2hEndTimestamp, joinEvents[1], 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(rootList, nullptr, 2, joinEvents));
    ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListEndGraphCapture(rootList, &graph, nullptr));

    ze_executable_graph_handle_t executableGraph{};
    ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListInstantiateGraph(graph, &executableGraph, nullptr));

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const uint64_t timestampValidBitsMask = levelzero.getTimestampValidBitsMask(levelzero.device);
    const auto getElapsedTime = [&](const uint64_t start, const uint64_t end) {
        return levelzero.getAbsoluteTimestampTime(start & timestampValidBitsMask, end & timestampValidBitsMask,
                                                  timerResolution, timestampValidBitsMask);
    };

    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListAppendGraph(replayList, executableGraph, nullptr, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(replayList, std::numeric_limits<uint64_t>::max()));

        const auto h2dTime = getElapsedTime(*startTimestamp, *h2dEndTimestamp);
        const auto d2hTime = getElapsedTime(*startTimestamp, *d2hEndTimestamp);
        statistics.pushValue(std::max(h2dTime, d2hTime), arguments.size * 2, typeSelector.getUnit(), typeSelector.getType(), "Total ");
        statistics.pushValue(h2dTime, arguments.size, typeSelector.getUnit(), typeSelector.getType(), "H2D ");
        statistics.pushValue(d2hTime, arguments.size, typeSelector.getUnit(), typeSelector.getType(), "D2H ");
    }

    ASSERT_ZE_RESULT_SUCCESS(graphApi.executableGraphDestroy(executableGraph));
    ASSERT_ZE_RESULT_SUCCESS(graphApi.graphDestroy(graph));
    for (const auto &joinEvent : joinEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(joinEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(forkEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestamps));

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, host1));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, host2));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, device1));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, device2));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(d2hList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(h2dList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(rootList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(replayList));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmConcurrentCopyGraph> registerTestCase(run, Api::L0, true);
