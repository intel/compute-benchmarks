/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ol/error.h"
#include "framework/ol/ol.h"
#include "framework/ol/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/queue_in_order_memcpy.h"

#include <gtest/gtest.h>

static TestResult run(const QueueInOrderMemcpyArguments &arguments,
                      Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    // Offload has no way to select the copy engine, and copy offload is an L0
    // driver hint.
    if (arguments.isCopyOnly || arguments.withCopyOffload) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    OlState ol;
    if (!OL::UsmHelper::isPlacementSupported(arguments.sourcePlacement,
                                             ol.hostDevice) ||
        !OL::UsmHelper::isPlacementSupported(arguments.destinationPlacement,
                                             ol.hostDevice)) {
        return TestResult::ApiNotCapable;
    }

    // Offload queues are always in-order.
    ol_queue_handle_t queue;
    ASSERT_OL_RESULT_SUCCESS(olCreateQueue(ol.context, ol.device, &queue));

    constexpr size_t alignment = 512;
    auto allocateMemory = [&](UsmMemoryPlacement placement, void **buffer) {
        if (placement == UsmMemoryPlacement::Device) {
            return OL::UsmHelper::allocateAligned(placement, ol.context,
                                                  ol.device, alignment,
                                                  arguments.size, buffer);
        }
        return OL::UsmHelper::allocate(placement, ol.context, ol.device,
                                       arguments.size, buffer);
    };
    void *source{}, *destination{};
    ASSERT_OL_RESULT_SUCCESS(
        allocateMemory(arguments.sourcePlacement, &source));
    ASSERT_OL_RESULT_SUCCESS(
        allocateMemory(arguments.destinationPlacement, &destination));
    const ol_device_handle_t sourceDevice = OL::UsmHelper::getMemcpyDevice(
        arguments.sourcePlacement, ol.device, ol.hostDevice);
    const ol_device_handle_t destinationDevice = OL::UsmHelper::getMemcpyDevice(
        arguments.destinationPlacement, ol.device, ol.hostDevice);

    for (auto i = 0u; i < arguments.iterations; ++i) {
        profiler.measureStart();
        for (auto j = 0u; j < arguments.count; ++j) {
            ASSERT_OL_RESULT_SUCCESS(olMemcpy(queue, destination,
                                              destinationDevice, source,
                                              sourceDevice, arguments.size));
        }
        ASSERT_OL_RESULT_SUCCESS(olSyncQueue(queue));
        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::deallocate(
        arguments.sourcePlacement, ol.context, source));
    ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::deallocate(
        arguments.destinationPlacement, ol.context, destination));
    ASSERT_OL_RESULT_SUCCESS(olDestroyQueue(queue));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<QueueInOrderMemcpy>
    registerTestCase(run, Api::OL);
