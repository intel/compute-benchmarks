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
#include "framework/utility/buffer_contents_helper.h"
#include "framework/utility/timer.h"

#include "definitions/usm_fill_immediate.h"

#include <vector>

static TestResult run(const UsmFillImmediateArguments &arguments,
                      Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond,
                                   MeasurementType::Cpu);

    // The L0 plugin doesn't implement olGetEventElapsedTime yet
    if (arguments.useEvents) {
        return TestResult::ApiNotCapable;
    }
    if (arguments.forceBlitter) {
        return TestResult::ApiNotCapable;
    }
    if (arguments.usmMemoryPlacement != UsmMemoryPlacement::Host &&
        arguments.usmMemoryPlacement != UsmMemoryPlacement::Device &&
        arguments.usmMemoryPlacement != UsmMemoryPlacement::Shared) {
        return TestResult::ApiNotCapable;
    }
    if (arguments.patternSize == 0) {
        return TestResult::InvalidArgs;
    }
    // olMemFill requires the fill size to be a multiple of the pattern size
    if (arguments.bufferSize % arguments.patternSize != 0) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(),
                                   typeSelector.getType());
        return TestResult::Nooped;
    }

    OlState ol;
    Timer timer;

    ol_queue_handle_t queue;
    ASSERT_OL_RESULT_SUCCESS(olCreateQueue(ol.context, ol.device, &queue));

    void *buffer{};
    ASSERT_OL_RESULT_SUCCESS(
        OL::UsmHelper::allocate(arguments.usmMemoryPlacement, ol.context,
                                ol.device, arguments.bufferSize, &buffer));

    void *contentsStaging{};
    ASSERT_OL_RESULT_SUCCESS(
        OL::UsmHelper::allocate(UsmMemoryPlacement::Host, ol.context,
                                ol.device, arguments.bufferSize,
                                &contentsStaging));
    BufferContentsHelper::fill(static_cast<uint8_t *>(contentsStaging),
                               arguments.bufferSize, arguments.contents);

    std::vector<uint8_t> pattern(arguments.patternSize);
    BufferContentsHelper::fill(pattern.data(), pattern.size(),
                               arguments.patternContents);

    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_OL_RESULT_SUCCESS(olMemcpy(queue, buffer, ol.device,
                                          contentsStaging, ol.device,
                                          arguments.bufferSize));
        ASSERT_OL_RESULT_SUCCESS(olSyncQueue(queue));

        timer.measureStart();
        ASSERT_OL_RESULT_SUCCESS(olMemFill(queue, buffer, pattern.size(),
                                           pattern.data(), arguments.bufferSize));
        ASSERT_OL_RESULT_SUCCESS(olSyncQueue(queue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), arguments.bufferSize,
                             typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::deallocate(
        UsmMemoryPlacement::Host, ol.context, contentsStaging));
    ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::deallocate(
        arguments.usmMemoryPlacement, ol.context, buffer));
    ASSERT_OL_RESULT_SUCCESS(olDestroyQueue(queue));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<UsmFillImmediate>
    registerTestCase(run, Api::OL);
