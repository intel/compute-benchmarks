/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/engine.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/immediate_cmdlist_completion.h"

#include <gtest/gtest.h>
#include <mutex>
#include <shared_mutex>
#include <thread>

struct ThreadSpecificData {
    ze_command_list_handle_t cmdList{};
    ze_event_handle_t event{};
    void *hostSrcMemory{};
    void *deviceDstMemory{};
    uint32_t maxMemoryAllocSize{};
    Timer timer{};
};

struct EngineInfo {
    uint32_t ordinal;
    uint32_t engineIndex;
};

static void issueToImmediateCmdList(ThreadSpecificData *threadData, std::shared_mutex *barrier) {
    std::shared_lock sharedLock(*barrier);
    threadData->timer.measureStart();
    zeCommandListAppendMemoryCopy(threadData->cmdList, threadData->deviceDstMemory,
                                  threadData->hostSrcMemory, threadData->maxMemoryAllocSize,
                                  threadData->event, 0, nullptr);

    zeEventHostSynchronize(threadData->event, std::numeric_limits<uint64_t>::max());
    threadData->timer.measureEnd();
}

static TestResult getEngineInfo(std::vector<EngineInfo> &supportedEngineInfo, LevelZero &levelzero, std::string_view engineGroup, const ImmediateCommandListCompletionArguments &arguments) {
    supportedEngineInfo.clear();

    uint32_t queueGroupPropertiesCount = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &queueGroupPropertiesCount, nullptr));
    FATAL_ERROR_IF(queueGroupPropertiesCount == 0, "No queue group properties found!");
    std::vector<ze_command_queue_group_properties_t> queueGroupProperties(queueGroupPropertiesCount);
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &queueGroupPropertiesCount, queueGroupProperties.data()));

    auto insertToEngineInfoList = [&supportedEngineInfo](auto &numQueues, auto &ordinal) {
        for (uint32_t j = 0; j < numQueues; j++) {
            supportedEngineInfo.push_back({ordinal, j});
        }
    };

    for (uint32_t i = 0; i < queueGroupPropertiesCount; i++) {
        if (engineGroup.compare("Compute") == 0) {
            if ((queueGroupProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
                insertToEngineInfoList(queueGroupProperties[i].numQueues, i);
            }
        } else {
            // Use all Copy Engines (main and link)
            if (queueGroupProperties[i].flags == ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) {
                insertToEngineInfoList(queueGroupProperties[i].numQueues, i);
            }
        }
    }

    // Apply the mask
    std::vector<EngineInfo>::iterator it = supportedEngineInfo.begin();
    std::bitset<maxNumberOfEngines> engineBitset = arguments.engineMask;
    uint32_t engineMaskPosition = 0;
    while (it != supportedEngineInfo.end()) {
        if (engineBitset.test(engineMaskPosition) == false) {
            it = supportedEngineInfo.erase(it);
        } else {
            ++it;
        }
        engineMaskPosition++;
    }
    return supportedEngineInfo.size() > 0 ? TestResult::Success : TestResult::Error;
}

static TestResult run(const ImmediateCommandListCompletionArguments &arguments, Statistics &statistics) {
    // Setup
    LevelZero levelzero;

    std::vector<EngineInfo> supportedEngineInfo{};
    std::string engineGroup = static_cast<const std::string &>(arguments.engineGroup);
    const auto status = getEngineInfo(supportedEngineInfo, levelzero, engineGroup, arguments);
    if (status != TestResult::Success) {
        return status;
    }

    const auto numberOfSupportedEngines = supportedEngineInfo.size();
    if (supportedEngineInfo.size() * arguments.threadsPerEngine > arguments.numberOfThreads) {
        return TestResult::InvalidArgs;
    }

    ze_command_queue_desc_t commandQueueDesc = {};
    commandQueueDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;

    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = arguments.numberOfThreads;
    ze_event_pool_handle_t eventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    std::vector<ThreadSpecificData> threadData(arguments.numberOfThreads);
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        commandQueueDesc.ordinal = supportedEngineInfo[i % numberOfSupportedEngines].ordinal;
        commandQueueDesc.index = supportedEngineInfo[i % numberOfSupportedEngines].engineIndex;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &threadData[i].cmdList));
        eventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &threadData[i].event));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.copySize, &threadData[i].hostSrcMemory));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.copySize, &threadData[i].deviceDstMemory));
        threadData[i].maxMemoryAllocSize = arguments.copySize;
    }

    std::shared_mutex barrier;
    // Warmup
    for (auto i = 0u; i < 5; i++) {
        std::unique_lock lock(barrier);
        std::vector<std::unique_ptr<std::thread>> threads;
        for (auto i = 0u; i < arguments.numberOfThreads; i++) {
            threads.push_back(std::unique_ptr<std::thread>(
                new std::thread(issueToImmediateCmdList, &threadData[i], &barrier)));
        }
        lock.unlock();
        for (auto i = 0u; i < arguments.numberOfThreads; i++) {
            threads[i]->join();
        }

        for (auto i = 0u; i < arguments.numberOfThreads; i++) {
            zeEventHostReset(threadData[i].event);
        }
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        std::unique_lock lock(barrier);
        std::vector<std::unique_ptr<std::thread>> threads;
        for (auto j = 0u; j < arguments.numberOfThreads; j++) {
            threads.push_back(std::unique_ptr<std::thread>(
                new std::thread(issueToImmediateCmdList, &threadData[j], &barrier)));
        }
        lock.unlock();
        for (auto j = 0u; j < arguments.numberOfThreads; j++) {
            threads[j]->join();
        }

        auto aggregatedThreadDuration = static_cast<std::chrono::high_resolution_clock::duration>(0);
        for (auto j = 0u; j < arguments.numberOfThreads; j++) {
            zeEventHostReset(threadData[j].event);
            aggregatedThreadDuration += threadData[j].timer.get();
        }
        const auto averageThreadDuration = aggregatedThreadDuration / arguments.numberOfThreads;
        statistics.pushValue(averageThreadDuration, MeasurementUnit::Microseconds, MeasurementType::Cpu, "Average Thread Duration");
    }

    // Cleanup
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Host, levelzero, threadData[i].hostSrcMemory));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, threadData[i].deviceDstMemory));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(threadData[i].event));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(threadData[i].cmdList));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ImmediateCommandListCompletion> registerTestCase(run, Api::L0);
