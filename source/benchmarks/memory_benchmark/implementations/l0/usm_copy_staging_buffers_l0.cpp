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
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_staging_buffers.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyStagingBuffersArguments &arguments, Statistics &statistics) {
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();

    LevelZero levelzero(queueProperties, contextProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (arguments.dstPlacement != UsmMemoryPlacement::Device && arguments.dstPlacement != UsmMemoryPlacement::Host) {
        return TestResult::InvalidArgs;
    }

    Timer timer;
    // Create src & dst buffers
    char *src{};
    char *dst{};
    size_t offset = arguments.size / arguments.chunks;

    if (arguments.dstPlacement == UsmMemoryPlacement::Device) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, reinterpret_cast<void **>(&dst)));
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, dst, arguments.size));
        src = new char[arguments.size];
    } else if (arguments.dstPlacement == UsmMemoryPlacement::Host) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, reinterpret_cast<void **>(&src)));
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, src, arguments.size));
        dst = new char[arguments.size];
    }

    //create staging buffers
    std::vector<void *> usmHost(arguments.chunks);
    for (auto i = 0u; i < arguments.chunks; i++) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, offset, &usmHost[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, usmHost[i], offset));
    }

    if (arguments.dstPlacement == UsmMemoryPlacement::Device) {
        memset(src, 0, arguments.size);
    } else {
        ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, src, arguments.size, BufferContents::Zeros, false));
    }

    // Create events
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 1;

    ze_event_pool_handle_t eventPool;
    ze_event_handle_t event{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_HOST};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    // Create command list
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.commandQueueDevice, queueProperties.selectedEngine);
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.commandQueueDevice, &commandQueueDesc->desc, &cmdList));

    // Warmup
    if (arguments.dstPlacement == UsmMemoryPlacement::Device) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dst, usmHost[0], offset, nullptr, 0, nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, usmHost[0], src, offset, nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto j = 0u; j < arguments.chunks; j++) {
            if (arguments.dstPlacement == UsmMemoryPlacement::Device) {
                memcpy(usmHost[j], src + offset * j, offset);
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dst + offset * j, usmHost[j], offset, nullptr, 0, nullptr));
            } else {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, usmHost[j], src + offset * j, offset, nullptr, 0, nullptr));
            }
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        if (arguments.dstPlacement == UsmMemoryPlacement::Host) {
            for (auto j = 0u; j < arguments.chunks; j++) {
                memcpy(dst + offset * j, usmHost[j], offset);
            }
        }

        timer.measureEnd();
        statistics.pushValue(timer.get(), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.dstPlacement == UsmMemoryPlacement::Device) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, dst, arguments.size));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, dst));
        delete[] src;
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, src, arguments.size));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, src));
        delete[] dst;
    }

    for (auto i = 0u; i < arguments.chunks; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, usmHost[i], offset));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Host, levelzero, usmHost[i]));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyStagingBuffers> registerTestCase(run, Api::L0);
