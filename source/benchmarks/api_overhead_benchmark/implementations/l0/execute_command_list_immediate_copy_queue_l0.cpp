/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_immediate_copy_queue.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateCopyQueueArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Timer timer;
    QueueProperties queueProperties = QueueProperties::create().setForceEngine(arguments.isCopyOnly ? Engine::Bcs : Engine::Ccs0).allowCreationFail();
    LevelZero levelzero = LevelZero{queueProperties};

    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    // Get ordinal
    uint32_t numQueueGroups = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &numQueueGroups, nullptr));
    if (numQueueGroups == 0) {
        return TestResult::DeviceNotCapable;
    }
    std::vector<ze_command_queue_group_properties_t> queueProps(numQueueGroups);
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &numQueueGroups,
                                                                    queueProps.data()));
    uint32_t copyOnlyOrdinal = std::numeric_limits<uint32_t>::max();
    uint32_t computeOrdinal = std::numeric_limits<uint32_t>::max();

    if (arguments.isCopyOnly) {
        for (uint32_t i = 0; i < numQueueGroups; i++) {
            if (!(queueProps[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) &&
                (queueProps[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
                copyOnlyOrdinal = i;
                break;
            }
        }
        if (copyOnlyOrdinal == std::numeric_limits<uint32_t>::max()) {
            return TestResult::DeviceNotCapable;
        }
    } else {
        for (uint32_t i = 0; i < numQueueGroups; i++) {
            if ((queueProps[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) &&
                (queueProps[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
                computeOrdinal = i;
                break;
            }
        }
        if (computeOrdinal == std::numeric_limits<uint32_t>::max()) {
            return TestResult::DeviceNotCapable;
        }
    }

    // Create event
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 1;
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ze_event_handle_t event{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    // Create buffers
    void *srcBuffer{}, *dstBuffer{};
    size_t bufferSize = 4096;
    const ze_host_mem_alloc_desc_t hostAllocDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocDesc, bufferSize, 0, &srcBuffer));
    memset(static_cast<uint8_t *>(srcBuffer), 0x7c, bufferSize);
    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, bufferSize, 0, levelzero.device, &dstBuffer));

    // Create an immediate command list
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    commandQueueDesc.ordinal = arguments.isCopyOnly ? copyOnlyOrdinal : computeOrdinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dstBuffer, srcBuffer, bufferSize, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dstBuffer, srcBuffer, bufferSize, event, 0, nullptr));

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }

    // Release
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, dstBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListImmediateCopyQueue> registerTestCase(run, Api::L0);
