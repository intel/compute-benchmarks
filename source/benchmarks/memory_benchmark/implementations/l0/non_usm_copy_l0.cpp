/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/usm_memory_placement.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/non_usm_copy.h"

#include <gtest/gtest.h>

static TestResult run(const NonUsmCopyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    if (arguments.sourcePlacement != UsmMemoryPlacement::Device && arguments.destinationPlacement != UsmMemoryPlacement::Device) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
    Timer timer;
    // Create an immediate command list
    ze_command_list_handle_t cmdList{};
    auto status = zeCommandListCreateImmediate(levelzero.context, levelzero.device, &defaultCommandQueueDesc, &cmdList);
    if (status != ZE_RESULT_SUCCESS) {
        return TestResult::DeviceNotCapable;
    }

    auto sourceNonUsm = isSharedSystemPointer(arguments.sourcePlacement);

    void *source{}, *destination{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &destination));
    if (sourceNonUsm) {
        memset(source, 1u, arguments.size);
    } else {
        memset(destination, 1u, arguments.size);
    }

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        if (arguments.reallocate) {
            if (sourceNonUsm) {
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
                memset(source, 1u, arguments.size);
            } else {
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, destination));
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &destination));
                memset(destination, 1u, arguments.size);
            }
        }
        if (arguments.prefetch) {
            if (sourceNonUsm) {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryPrefetch(cmdList, source, arguments.size));
            } else {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryPrefetch(cmdList, destination, arguments.size));
            }
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        if (arguments.updateOnHost) {
            if (sourceNonUsm) {
                memset(source, (rand() & 0xff), arguments.size);
            } else {
                memset(destination, (rand() & 0xff), arguments.size);
            }
        }

        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, destination));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<NonUsmCopy> registerTestCase(run, Api::L0);
