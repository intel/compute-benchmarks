/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_bidirectional_copy.h"

#include <gtest/gtest.h>

static TestResult run(const UsmBidirectionalCopyArguments &arguments, Statistics &statistics) {
    ContextProperties contextProperties = ContextProperties::create().create().setDeviceSelection(DeviceSelection::Tile0 | DeviceSelection::Tile1).allowCreationFail();
    LevelZero levelzero(contextProperties);
    if (levelzero.context == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    Timer timer;

    // Create buffers
    void *tile0Src{}, *tile0Dst{};
    void *tile1Src{}, *tile1Dst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(DeviceSelection::Tile0, levelzero, arguments.size, &tile0Src));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(DeviceSelection::Tile0, levelzero, arguments.size, &tile0Dst));

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(DeviceSelection::Tile1, levelzero, arguments.size, &tile1Src));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(DeviceSelection::Tile1, levelzero, arguments.size, &tile1Dst));

    // Make the buffers resident
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(DeviceSelection::Tile0), tile0Src, arguments.size));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(DeviceSelection::Tile0), tile0Dst, arguments.size));

    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(DeviceSelection::Tile1), tile1Src, arguments.size));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.getDevice(DeviceSelection::Tile1), tile1Dst, arguments.size));

    // Create command lists and queues
    QueueProperties tile0QueueProperties = QueueProperties::create().setDeviceSelection(DeviceSelection::Tile0).setForceBlitter(arguments.forceBlitter).allowCreationFail();
    QueueFamiliesHelper::QueueDesc tile0QueueDesc = levelzero.createQueue(tile0QueueProperties);
    ze_command_list_desc_t tile0CmdListDesc{};
    tile0CmdListDesc.commandQueueGroupOrdinal = tile0QueueDesc.desc.ordinal;
    ze_command_list_handle_t tile0CmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.commandQueueDevice, &tile0CmdListDesc, &tile0CmdList));
    ze_command_queue_handle_t tile0CmdQueue = tile0QueueDesc.queue;

    QueueProperties tile1QueueProperties = QueueProperties::create().setDeviceSelection(DeviceSelection::Tile1).setForceBlitter(arguments.forceBlitter).allowCreationFail();
    QueueFamiliesHelper::QueueDesc tile1QueueDesc = levelzero.createQueue(tile1QueueProperties);
    ze_command_list_desc_t tile1CmdListDesc{};
    tile1CmdListDesc.commandQueueGroupOrdinal = tile1QueueDesc.desc.ordinal;
    ze_command_list_handle_t tile1CmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.commandQueueDevice, &tile1CmdListDesc, &tile1CmdList));
    ze_command_queue_handle_t tile1CmdQueue = tile1QueueDesc.queue;

    // append copies on both directions
    if (arguments.write) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(tile0CmdList, tile1Dst, tile0Src, arguments.size, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(tile1CmdList, tile0Dst, tile1Src, arguments.size, nullptr, 0, nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(tile0CmdList, tile0Dst, tile1Src, arguments.size, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(tile1CmdList, tile1Dst, tile0Src, arguments.size, nullptr, 0, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(tile0CmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(tile1CmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(tile0CmdQueue, 1, &tile0CmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(tile1CmdQueue, 1, &tile1CmdList, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(tile0CmdQueue, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(tile1CmdQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(tile0CmdQueue, 1, &tile0CmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(tile1CmdQueue, 1, &tile1CmdList, nullptr));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(tile0CmdQueue, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(tile1CmdQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.size * 2, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
    }

    // Evict buffers
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(DeviceSelection::Tile0), tile0Src, arguments.size));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(DeviceSelection::Tile0), tile0Dst, arguments.size));

    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(DeviceSelection::Tile1), tile1Src, arguments.size));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.getDevice(DeviceSelection::Tile1), tile1Dst, arguments.size));

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(tile0CmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(tile1CmdList));

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, tile0Src));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, tile0Dst));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, tile1Src));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, tile1Dst));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmBidirectionalCopy> registerTestCase(run, Api::L0);
