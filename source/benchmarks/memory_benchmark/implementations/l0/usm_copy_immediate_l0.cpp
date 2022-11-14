/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyImmediateArguments &arguments, Statistics &statistics) {
    if (arguments.sourcePlacement == UsmMemoryPlacement::NonUsmMapped ||
        arguments.destinationPlacement == UsmMemoryPlacement::NonUsmMapped) {
        return TestResult::ApiNotCapable;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
        (arguments.sourcePlacement == UsmMemoryPlacement::NonUsmImported ||
         arguments.destinationPlacement == UsmMemoryPlacement::NonUsmImported));

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    // Create buffers
    void *source{}, *destination{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &destination));

    if (arguments.sourcePlacement != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, source, arguments.size));
    }
    if (arguments.destinationPlacement != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, destination, arguments.size));
    }

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    // Create an immediate command list
    ze_command_list_handle_t cmdList{};
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, queueProperties.selectedEngine);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdList));

    // Fill buffer
    if (arguments.sourcePlacement != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, source, arguments.size, arguments.contents));
    }
    if (arguments.destinationPlacement != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, destination, arguments.size, arguments.contents));
    }

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        ze_kernel_timestamp_result_t timestampResult{};
        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
        auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
        commandTime *= timerResolution;
        statistics.pushValue(commandTime, arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }

    // Evict buffers
    if (arguments.destinationPlacement != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, destination, arguments.size));
    }
    if (arguments.sourcePlacement != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, source, arguments.size));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, destination));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyImmediate> registerTestCase(run, Api::L0);
