/*
 * Copyright (C) 2023 Intel Corporation
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
    std::cout << "Hello world";
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (arguments.srcptr == UsmMemoryPlacement::NonUsmMapped ||
        arguments.dstptr == UsmMemoryPlacement::NonUsmMapped) {
        return TestResult::ApiNotCapable;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
        (arguments.srcptr == UsmMemoryPlacement::NonUsmImported ||
         arguments.dstptr == UsmMemoryPlacement::NonUsmImported));

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

    // Create buffers
    void *source{}, *destination{};
    const ze_copy_region_t src = {(uint32_t)arguments.srcRegion[0], (uint32_t)arguments.srcRegion[1], (uint32_t)arguments.srcRegion[2], (uint32_t)arguments.srcOrigin[0], (uint32_t)arguments.srcOrigin[1], (uint32_t)arguments.srcOrigin[2]};
    const ze_copy_region_t dst = {(uint32_t)arguments.dstRegion[0], (uint32_t)arguments.dstRegion[1], (uint32_t)arguments.dstRegion[2], (uint32_t)arguments.dstOrigin[0], (uint32_t)arguments.dstOrigin[1], (uint32_t)arguments.dstOrigin[2]};
    const size_t size = arguments.srcRegion[0] * arguments.srcRegion[1] * arguments.srcRegion[2];
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.srcptr, levelzero, size, &source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dstptr, levelzero, size, &destination));

    if (arguments.srcptr != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, source, size));
    }
    if (arguments.dstptr != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, destination, size));
    }

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useEvents) {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopyRegion(cmdList, destination, &dst, arguments.dstRegion[0], arguments.dstRegion[0]*arguments.dstRegion[1],
                                                             source, &src, arguments.srcRegion[0], arguments.srcRegion[0]*arguments.srcRegion[1],
                                                             event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    if (arguments.dstptr != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, destination, size));
    }
    if (arguments.srcptr != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, source, size));
    }

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        if (!arguments.reuseCommandList) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopyRegion(cmdList, destination, &dst, arguments.dstRegion[0], arguments.dstRegion[0]*arguments.dstRegion[1],
                                                             source, &src, arguments.srcRegion[0], arguments.srcRegion[0]*arguments.srcRegion[1],
                                                             event, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
            commandTime *= timerResolution;
            statistics.pushValue(commandTime, size, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), size, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Evict buffers
    if (arguments.dstptr != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, destination, size));
    }
    if (arguments.srcptr != UsmMemoryPlacement::NonUsm) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, source, size));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.srcptr, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.dstptr, levelzero, destination));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyRegion> registerTestCase(run, Api::L0);
