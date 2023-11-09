/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_concurrent_multiple_blits.h"

#include <gtest/gtest.h>

struct PerBlitterWorkInfo {
    ze_command_list_handle_t list;
    std::string name;
    ze_event_handle_t completionEvent{};
    void *copySrc = nullptr;
    void *copyDst = nullptr;
    size_t size;
};

struct TransferBuffer {
    void *hostBuffer = nullptr;
    void *deviceBuffer = nullptr;
    LevelZero &levelzero;

    TransferBuffer() = delete;
    TransferBuffer(LevelZero &levelzero) : levelzero(levelzero) {}

    TestResult allocate(size_t size) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Host, levelzero, size, &hostBuffer));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmRuntimeMemoryPlacement::Device, levelzero, size, &deviceBuffer));
        return TestResult::Success;
    }

    TestResult release() {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostBuffer));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, deviceBuffer));
        return TestResult::Success;
    }
};

static TestResult startCopyOnBlitters(std::vector<PerBlitterWorkInfo> &blitterWorkInfos, ze_event_handle_t &synchronizedStartEvent) {
    for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(workInfo.list,
                                                               workInfo.copyDst,
                                                               workInfo.copySrc,
                                                               workInfo.size,
                                                               workInfo.completionEvent, 1, &synchronizedStartEvent));
    }
    return TestResult::Success;
}

static TestResult waitForAllBlittersToComplete(std::vector<PerBlitterWorkInfo> &blitterWorkInfos) {
    for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(workInfo.completionEvent,
                                                        std::numeric_limits<uint64_t>::max()));
    }
    return TestResult::Success;
}

static TestResult prepareWorkForBlitters(std::vector<PerBlitterWorkInfo> &blitterWorkInfos,
                                         bool isH2d, const BcsBitmaskArgument &blitter,
                                         uint32_t mainCopyOrdinal, uint32_t linkCopyOrdinal,
                                         std::vector<ze_command_queue_group_properties_t> &queueProperties,
                                         void *srcBuffer, void *dstBuffer, uint32_t sizeToCopy,
                                         ze_event_pool_handle_t &eventPool, uint32_t &eventIndex,
                                         LevelZero &levelzero) {

    uint32_t ordinal = 0;
    uint32_t index = 0;

    auto totalSize = sizeToCopy;
    auto engineCount = blitter.getEnabledBits().size();

    for (size_t blitterIndex : blitter.getEnabledBits()) {
        const bool isMainCopyEngine = blitterIndex == 0;
        if (isMainCopyEngine) {
            if (mainCopyOrdinal == std::numeric_limits<uint32_t>::max()) {
                return TestResult::DeviceNotCapable;
            }
            ordinal = mainCopyOrdinal;
            index = 0;
        } else {
            if (linkCopyOrdinal == std::numeric_limits<uint32_t>::max() || blitterIndex > queueProperties[linkCopyOrdinal].numQueues) {
                return TestResult::DeviceNotCapable;
            }
            ordinal = linkCopyOrdinal;
            index = static_cast<uint32_t>(blitterIndex - queueProperties[mainCopyOrdinal].numQueues);
        }

        ze_command_queue_desc_t cmdQueueDesc = {};
        cmdQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
        cmdQueueDesc.ordinal = ordinal;
        cmdQueueDesc.index = index;

        PerBlitterWorkInfo info{};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context,
                                                              levelzero.device,
                                                              &cmdQueueDesc,
                                                              &info.list));

        const Engine engine = EngineHelper::getBlitterEngineFromIndex(blitterIndex);
        info.name = EngineHelper::getEngineName(engine);
        info.name.append(isH2d ? "-h2d" : "-d2h");

        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = eventIndex;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &info.completionEvent));

        auto localSize = totalSize / engineCount;
        info.copySrc = static_cast<char *>(srcBuffer) + (sizeToCopy - totalSize);
        info.copyDst = static_cast<char *>(dstBuffer) + (sizeToCopy - totalSize);
        info.size = localSize;

        blitterWorkInfos.push_back(info);

        eventIndex++;
        totalSize -= static_cast<uint32_t>(localSize);
        engineCount--;
    }
    return TestResult::Success;
}

uint64_t gettotalBytesTransferred(std::vector<PerBlitterWorkInfo> &blitterWorkInfos) {
    uint64_t memTransferred = 0;
    for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
        memTransferred += workInfo.size;
    }
    return memTransferred;
}

static TestResult run(const UsmCopyConcurrentMultipleBlitsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);

    auto result = TestResult::Success;

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    LevelZero levelzero;

    uint32_t numQueueGroups = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device,
                                                                    &numQueueGroups,
                                                                    nullptr));
    if (numQueueGroups == 0) {
        return TestResult::DeviceNotCapable;
    }

    uint32_t h2dTransferDirectionEnabled = arguments.h2dBlitters.getEnabledBits().size() > 0 ? 1 : 0;
    uint32_t d2hTransferDirectionEnabled = arguments.d2hBlitters.getEnabledBits().size() > 0 ? 1 : 0;

    uint64_t memoryRequired = arguments.size * (h2dTransferDirectionEnabled * 2 + d2hTransferDirectionEnabled * 2);
    ze_device_properties_t deviceProperties{};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    if (deviceProperties.maxMemAllocSize < memoryRequired) {
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

    // Create buffers
    TransferBuffer h2dTransferBuffers(levelzero);
    TransferBuffer d2hTransferBuffers(levelzero);
    result = h2dTransferBuffers.allocate(arguments.size);
    if (result != TestResult::Success) {
        return result;
    }
    result = d2hTransferBuffers.allocate(arguments.size);
    if (result != TestResult::Success) {
        return result;
    }

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    // "2" to consider both d2h and h2d case and 1 for the synchronized starting event
    eventPoolDesc.count = maxNumberOfEngines * 2 + 1;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    // Create start event
    ze_event_handle_t synchronizedStartEvent;
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &synchronizedStartEvent));

    uint32_t eventIndex = 1;
    // Create selected blitter lists
    std::vector<PerBlitterWorkInfo> blitterWorkInfos;

    result = prepareWorkForBlitters(blitterWorkInfos, true, arguments.h2dBlitters, mainCopyOrdinal, linkCopyOrdinal,
                                    queueProperties, h2dTransferBuffers.hostBuffer, h2dTransferBuffers.deviceBuffer, static_cast<uint32_t>(arguments.size),
                                    eventPool, eventIndex, levelzero);
    if (result != TestResult::Success) {
        return result;
    }
    result = prepareWorkForBlitters(blitterWorkInfos, false, arguments.d2hBlitters, mainCopyOrdinal, linkCopyOrdinal,
                                    queueProperties, d2hTransferBuffers.deviceBuffer, d2hTransferBuffers.hostBuffer, static_cast<uint32_t>(arguments.size),
                                    eventPool, eventIndex, levelzero);
    if (result != TestResult::Success) {
        return result;
    }

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(synchronizedStartEvent));
    result = startCopyOnBlitters(blitterWorkInfos, synchronizedStartEvent);
    if (result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(synchronizedStartEvent));
    result = waitForAllBlittersToComplete(blitterWorkInfos);
    if (result != TestResult::Success) {
        return result;
    }
    for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(workInfo.completionEvent));
    }

    // Benchmark
    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const auto totalBytesTransferred = gettotalBytesTransferred(blitterWorkInfos);
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(synchronizedStartEvent));
        result = startCopyOnBlitters(blitterWorkInfos, synchronizedStartEvent);
        if (result != TestResult::Success) {
            return result;
        }
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(synchronizedStartEvent));
        result = waitForAllBlittersToComplete(blitterWorkInfos);
        if (result != TestResult::Success) {
            return result;
        }
        timer.measureEnd();

        // Report individual engines results and get time delta
        std::chrono::nanoseconds endGpuTime{};
        std::chrono::nanoseconds startGpuTime = std::chrono::nanoseconds::duration::max();

        for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(workInfo.completionEvent, &timestampResult));
            auto startTime = std::chrono::nanoseconds(timestampResult.global.kernelStart * timerResolution);
            auto endTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd * timerResolution);
            auto commandTime = endTime - startTime;
            startGpuTime = std::min(startTime, startGpuTime);
            endGpuTime = std::max(endTime, endGpuTime);
            statistics.pushValue(commandTime, workInfo.size, typeSelector.getUnit(), typeSelector.getType(), workInfo.name);
        }

        // Report total results
        statistics.pushValue(endGpuTime - startGpuTime, totalBytesTransferred, typeSelector.getUnit(), typeSelector.getType(), "Total (Gpu)");
        statistics.pushValue(timer.get(), totalBytesTransferred, typeSelector.getUnit(), MeasurementType::Cpu, "Total (Cpu)");

        for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(workInfo.completionEvent));
        }
    }

    for (PerBlitterWorkInfo &workInfo : blitterWorkInfos) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(workInfo.completionEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(workInfo.list));
    }

    h2dTransferBuffers.release();
    d2hTransferBuffers.release();

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(synchronizedStartEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyConcurrentMultipleBlits> registerTestCase(run, Api::L0);
