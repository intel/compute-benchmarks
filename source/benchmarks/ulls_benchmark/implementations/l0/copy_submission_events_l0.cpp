/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/bit_operations_helper.h"
#include "framework/utility/timer.h"

#include "definitions/copy_submission_events.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

using Clock = std::chrono::high_resolution_clock;

static TestResult run(const CopySubmissionEventsArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties;
    queueProperties.disable();
    LevelZero levelzero(queueProperties);
    constexpr static auto bufferSize = 2097152u;

    auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, arguments.engine);
    if (nullptr == queueDesc) {
        return TestResult::DeviceNotCapable;
    }
    levelzero.commandQueue = levelzero.createQueue(levelzero.device, queueDesc->desc);
    levelzero.commandQueueDesc = queueDesc->desc;

    const uint64_t timerResolution = levelzero.getTimerResoultion(levelzero.device);
    const uint32_t timestampValidBits = levelzero.getTimestampValidBits(levelzero.device);
    const uint32_t kernelTimestampValidBits = levelzero.getKernelTimestampValidBits(levelzero.device);
    const uint32_t sharedTimestampValidBits = std::min(timestampValidBits, kernelTimestampValidBits);

    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC, nullptr, ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED};
    void *hostMemory = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &hostMemory));

    const ze_device_mem_alloc_desc_t allocationDescDevice{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC, nullptr, ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED};
    void *destination = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &allocationDescDevice, bufferSize, 0, levelzero.device, &destination));

    // Create event for profiling
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, 1};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));

    const ze_event_desc_t eventDesc{};
    ze_event_handle_t hEvent;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &hEvent));

    // Create command list
    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, hostMemory, bufferSize, hEvent, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    uint64_t hostEnqueueTimestamp = 0;
    uint64_t deviceEnqueueTimestamp = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetGlobalTimestamps(levelzero.device, &hostEnqueueTimestamp, &deviceEnqueueTimestamp));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, std::numeric_limits<uint64_t>::max()));

    ze_kernel_timestamp_result_t deviceStartTimestamp;
    ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(hEvent, &deviceStartTimestamp));

    uint64_t truncatedDeviceEnqueueTimestamp = BitHelper::isolateLowerNBits(deviceEnqueueTimestamp, sharedTimestampValidBits);
    uint64_t truncatedKernelStartTimestamp = BitHelper::isolateLowerNBits(deviceStartTimestamp.global.kernelStart, sharedTimestampValidBits);
    EXPECT_GT(truncatedKernelStartTimestamp, truncatedDeviceEnqueueTimestamp);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(hEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetGlobalTimestamps(levelzero.device, &hostEnqueueTimestamp, &deviceEnqueueTimestamp));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, std::numeric_limits<uint64_t>::max()));

        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(hEvent, &deviceStartTimestamp));
        truncatedDeviceEnqueueTimestamp = BitHelper::isolateLowerNBits(deviceEnqueueTimestamp, sharedTimestampValidBits);
        truncatedKernelStartTimestamp = BitHelper::isolateLowerNBits(deviceStartTimestamp.global.kernelStart, sharedTimestampValidBits);

        std::chrono::nanoseconds submissionTime = levelzero.getAbsoluteSubmissionTime(truncatedKernelStartTimestamp,
                                                                                      truncatedDeviceEnqueueTimestamp,
                                                                                      timerResolution);

        statistics.pushValue(submissionTime, MeasurementUnit::Microseconds, MeasurementType::Gpu);
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostMemory));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, destination));

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopySubmissionEvents> registerTestCase(run, Api::L0);
