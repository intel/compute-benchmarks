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

#include "blit_size_assigner.h"
#include "definitions/usm_fill_multiple_blits.h"

#include <gtest/gtest.h>

static TestResult run(const UsmFillMultipleBlitsArguments &arguments, Statistics &statistics) {
    if (arguments.memoryPlacement == UsmMemoryPlacement::NonUsmMapped) {
        return TestResult::ApiNotCapable;
    }

    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
        (arguments.memoryPlacement == UsmMemoryPlacement::NonUsmImported));
    LevelZero levelzero(QueueProperties::create(),
                        ContextProperties::create(),
                        extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    uint32_t numQueueGroups = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device,
                                                                    &numQueueGroups,
                                                                    nullptr));
    if (numQueueGroups == 0) {
        return TestResult::DeviceNotCapable;
    }

    std::vector<ze_command_queue_group_properties_t> queueProperties(numQueueGroups);
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device,
                                                                    &numQueueGroups,
                                                                    queueProperties.data()));

    uint32_t mainCopyOrdinal = std::numeric_limits<uint32_t>::max();
    uint32_t linkCopyOrdinal = std::numeric_limits<uint32_t>::max();
    for (uint32_t i = 0; i < numQueueGroups; i++) {
        if ((queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
            (queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
            if (queueProperties[i].numQueues == 1) {
                mainCopyOrdinal = i;
            } else {
                linkCopyOrdinal = i;
            }
        }
    }

    // Create selected blitter queues
    struct PerQueueData {
        ze_command_queue_handle_t queue;
        ze_command_list_handle_t list;
        std::string name;
        bool isMainCopyEngine;
        ze_event_handle_t event{};
        void *fillDst;
        size_t fillSize = 0;
    };
    std::vector<PerQueueData> queues;

    BlitSizeAssigner blitSizeAssigner{arguments.size};

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = maxNumberOfEngines;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_command_queue_desc_t cmdQueueDesc = {};
    cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

    ze_command_list_desc_t cmdListDesc{};

    uint32_t eventIndex = 0;
    for (size_t blitterIndex : arguments.blitters.getEnabledBits()) {
        const bool isMainCopyEngine = blitterIndex == 0;
        if (isMainCopyEngine) {
            if (mainCopyOrdinal == std::numeric_limits<uint32_t>::max()) {
                return TestResult::DeviceNotCapable;
            }
            cmdQueueDesc.ordinal = mainCopyOrdinal;
            cmdQueueDesc.index = 0;
            blitSizeAssigner.addMainCopyEngine();
        } else {
            if (linkCopyOrdinal == std::numeric_limits<uint32_t>::max() || blitterIndex >= queueProperties[linkCopyOrdinal].numQueues) {
                return TestResult::DeviceNotCapable;
            }
            cmdQueueDesc.ordinal = linkCopyOrdinal;
            cmdQueueDesc.index = static_cast<uint32_t>(blitterIndex - queueProperties[mainCopyOrdinal].numQueues);
            blitSizeAssigner.addLinkCopyEngine();
        }

        ze_command_queue_handle_t queue;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueCreate(levelzero.context,
                                                      levelzero.device,
                                                      &cmdQueueDesc,
                                                      &queue));

        cmdListDesc.commandQueueGroupOrdinal = cmdQueueDesc.ordinal;
        ze_command_list_handle_t list;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context,
                                                     levelzero.device,
                                                     &cmdListDesc,
                                                     &list));

        const Engine engine = EngineHelper::getBlitterEngineFromIndex(blitterIndex);
        const std::string queueName = EngineHelper::getEngineName(engine);

        ze_event_handle_t event{};
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = eventIndex++;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

        queues.push_back(PerQueueData{queue, list, queueName, isMainCopyEngine, event});
    }

    // Create buffers
    void *dstBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.memoryPlacement,
                                                 levelzero,
                                                 arguments.size,
                                                 &dstBuffer));

    // Calculate copyOffset and copySize for each copy engine
    for (auto i = 0u; i < queues.size(); i++) {
        const auto [offset, size] = blitSizeAssigner.getSpaceForBlit(queues[i].isMainCopyEngine);
        queues[i].fillDst = static_cast<char *>(dstBuffer) + offset;
        queues[i].fillSize = size;
    }

    blitSizeAssigner.validate();

    // Create pattern
    const auto pattern = std::make_unique<uint8_t[]>(arguments.patternSize);
    BufferContentsHelperL0::fill(pattern.get(), arguments.patternSize, arguments.patternContents);

    // Append commands
    for (PerQueueData &queue : queues) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(queue.list,
                                                               queue.fillDst,
                                                               pattern.get(),
                                                               arguments.patternSize,
                                                               queue.fillSize,
                                                               queue.event, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(queue.list));
    }

    // Warmup
    for (PerQueueData &queue : queues) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queue.queue,
                                                                   1,
                                                                   &queue.list,
                                                                   nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue.queue,
                                                           std::numeric_limits<uint64_t>::max()));
    }

    // Benchmark
    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResoultion(levelzero.device);
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (PerQueueData &queue : queues) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queue.queue,
                                                                       1,
                                                                       &queue.list,
                                                                       nullptr));
        }
        for (PerQueueData &queue : queues) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queue.queue,
                                                               std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();

        // Report individual engines results and get time delta
        std::chrono::nanoseconds endGpuTime{};
        std::chrono::nanoseconds startGpuTime = std::chrono::nanoseconds::duration::max();

        for (PerQueueData &queue : queues) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(queue.event, &timestampResult));
            auto startTime = std::chrono::nanoseconds(timestampResult.global.kernelStart * timerResolution);
            auto endTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd * timerResolution);
            auto commandTime = endTime - startTime;
            startGpuTime = std::min(startTime, startGpuTime);
            endGpuTime = std::max(endTime, endGpuTime);
            statistics.pushValue(commandTime, queue.fillSize, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu, queue.name);
        }

        // Report total results
        statistics.pushValue(endGpuTime - startGpuTime, arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu, "Total (Gpu)");
        statistics.pushValue(timer.get(), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu, "Total (Cpu)");
    }

    for (PerQueueData &queue : queues) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(queue.event));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(queue.list));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueDestroy(queue.queue));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, dstBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmFillMultipleBlits> registerTestCase(run, Api::L0);
