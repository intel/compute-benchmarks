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

#include "definitions/usm_concurrent_copy_contended.h"

#include <algorithm>
#include <limits>

using namespace L0;

struct ContendedCopy {
    ze_command_list_handle_t cmdList{};
    void *source{};
    void *destination{};
    size_t size = 0;
    ze_event_handle_t completionEvent{};
};

static int64_t timestampDifference(uint64_t end, uint64_t start, uint64_t validBitsMask) {
    const uint64_t difference = (end - start) & validBitsMask;
    if (difference > validBitsMask / 2) {
        return static_cast<int64_t>(difference) - static_cast<int64_t>(validBitsMask) - 1;
    }
    return static_cast<int64_t>(difference);
}

static double getOverlapPercent(const ze_kernel_timestamp_data_t &measured, const ze_kernel_timestamp_data_t &other, uint64_t validBitsMask) {
    const int64_t measuredEnd = timestampDifference(measured.kernelEnd, measured.kernelStart, validBitsMask);
    if (measuredEnd <= 0) {
        return 0.0;
    }
    const int64_t otherStart = timestampDifference(other.kernelStart, measured.kernelStart, validBitsMask);
    const int64_t otherEnd = timestampDifference(other.kernelEnd, measured.kernelStart, validBitsMask);
    const int64_t overlap = std::min(measuredEnd, otherEnd) - std::max(int64_t{0}, otherStart);
    return 100.0 * static_cast<double>(std::max(int64_t{0}, overlap)) / static_cast<double>(measuredEnd);
}

static TestResult run(const UsmConcurrentCopyContendedArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero(QueueProperties::create().disable());

    const auto h2dQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.h2dEngine);
    const auto d2hQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.d2hEngine);
    if (h2dQueueDesc == nullptr || d2hQueueDesc == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    const bool measureH2D = arguments.measuredDirection == CopyDirection::HostToDevice;
    ContendedCopy h2d{};
    ContendedCopy d2h{};
    h2d.size = arguments.size;
    d2h.size = arguments.size;
    ContendedCopy &measured = measureH2D ? h2d : d2h;
    ContendedCopy &other = measureH2D ? d2h : h2d;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &h2dQueueDesc->desc, &h2d.cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &d2hQueueDesc->desc, &d2h.cmdList));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, h2d.size, &h2d.source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, h2d.size, &h2d.destination));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, d2h.size, &d2h.source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, d2h.size, &d2h.destination));

    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    if (arguments.useEvents) {
        eventPoolDesc.flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    }
    eventPoolDesc.count = 3u;
    ze_event_pool_handle_t eventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.index = 0u;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &h2d.completionEvent));
    eventDesc.index = 1u;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &d2h.completionEvent));
    eventDesc.index = 2u;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;
    ze_event_handle_t gateEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &gateEvent));

    const uint64_t validBitsMask = levelzero.getKernelTimestampValidBitsMask(levelzero.device);
    const std::string measuredLabel = measureH2D ? "H2D" : "D2H";
    Timer timer;
    for (auto i = 0u; i < arguments.iterations; i++) {
        for (ContendedCopy *copy : {&measured, &other}) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(copy->cmdList, copy->destination, copy->source, copy->size, copy->completionEvent, 1, &gateEvent));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(gateEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(measured.completionEvent, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(other.completionEvent, std::numeric_limits<uint64_t>::max()));

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t measuredTimestamp{};
            ze_kernel_timestamp_result_t otherTimestamp{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(measured.completionEvent, &measuredTimestamp));
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(other.completionEvent, &otherTimestamp));
            statistics.pushValue(levelzero.getAbsoluteKernelExecutionTime(measuredTimestamp.global), arguments.size, typeSelector.getUnit(), typeSelector.getType(), measuredLabel);
            statistics.pushPercentage(getOverlapPercent(measuredTimestamp.global, otherTimestamp.global, validBitsMask), MeasurementUnit::Percentage, MeasurementType::Gpu, "Overlap");
        } else {
            statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType(), measuredLabel);
        }

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(h2d.completionEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(d2h.completionEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(gateEvent));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(gateEvent));
    for (ContendedCopy *copy : {&h2d, &d2h}) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(copy->completionEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(copy->cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, copy->source));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, copy->destination));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmConcurrentCopyContended> registerTestCase(run, Api::L0);
