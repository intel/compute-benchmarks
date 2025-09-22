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

#include "definitions/usm_copy_region.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyRegionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (arguments.sourcePlacement == UsmMemoryPlacement::NonUsmMapped ||
        arguments.destinationPlacement == UsmMemoryPlacement::NonUsmMapped) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
        (requiresImport(arguments.sourcePlacement) ||
         requiresImport(arguments.destinationPlacement)));

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    // Create buffers
    void *source{}, *destination{};
    const ze_copy_region_t reg = {(uint32_t)arguments.origin[0], (uint32_t)arguments.origin[1], (uint32_t)arguments.origin[2], (uint32_t)arguments.region[0], (uint32_t)arguments.region[1], (uint32_t)arguments.region[2]};
    auto copySize = arguments.region[0] * arguments.region[1] * arguments.region[2];
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &destination));

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ze_event_pool_flags_t eventPoolFlags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    if (arguments.useEvents) {
        eventPoolFlags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    }
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = eventPoolFlags;
    eventPoolDesc.count = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    // Create an immediate command list
    ze_command_list_handle_t cmdList{};
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, queueProperties.selectedEngine);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopyRegion(cmdList, destination, &reg, static_cast<uint32_t>(arguments.region[0]), static_cast<uint32_t>(arguments.region[0] * arguments.region[1]),
                                                                 source, &reg, static_cast<uint32_t>(arguments.region[0]), static_cast<uint32_t>(arguments.region[0] * arguments.region[1]),
                                                                 event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopyRegion(cmdList, destination, &reg, static_cast<uint32_t>(arguments.region[0]), static_cast<uint32_t>(arguments.region[0] * arguments.region[1]),
                                                                     source, &reg, static_cast<uint32_t>(arguments.region[0]), static_cast<uint32_t>(arguments.region[0] * arguments.region[1]),
                                                                     event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
            commandTime *= timerResolution;
            statistics.pushValue(commandTime, copySize, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), copySize, typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, destination));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyRegion> registerTestCase(run, Api::L0);
