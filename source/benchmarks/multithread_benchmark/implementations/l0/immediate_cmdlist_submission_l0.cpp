/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/engine.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/immediate_cmdlist_submission.h"

#include <gtest/gtest.h>
#include <mutex>
#include <shared_mutex>
#include <thread>

struct ThreadSpecificData {
    ze_command_list_handle_t cmdList{};
    ze_event_handle_t event{};
    void *hostMemory = nullptr;
    Timer timer{};
    ze_kernel_handle_t kernel{};
};

struct EngineInfo {
    uint32_t ordinal;
    uint32_t engineIndex;
};

static void issueToImmediateCmdList(ThreadSpecificData *threadData, std::shared_mutex *barrier) {
    const ze_group_count_t groupCount{1, 1, 1};
    volatile uint64_t *volatileBuffer = static_cast<uint64_t *>(threadData->hostMemory);
    *volatileBuffer = 0;
    _mm_clflush(threadData->hostMemory);

    std::shared_lock sharedLock(*barrier);
    threadData->timer.measureStart();
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(threadData->cmdList, threadData->kernel, &groupCount, threadData->event, 0, nullptr));
    while (*volatileBuffer != 1) {
    }
    threadData->timer.measureEnd();
    EXPECT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(threadData->event, std::numeric_limits<uint64_t>::max()));
}

static TestResult getComputeEngineInfo(std::vector<EngineInfo> &supportedEngineInfo, LevelZero &levelzero) {
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
        if ((queueGroupProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            insertToEngineInfoList(queueGroupProperties[i].numQueues, i);
        }
    }
    return supportedEngineInfo.size() > 0 ? TestResult::Success : TestResult::Error;
}

static TestResult run(const ImmediateCommandListSubmissionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    // Setup
    LevelZero levelzero;

    std::vector<EngineInfo> supportedEngineInfo{};
    const auto status = getComputeEngineInfo(supportedEngineInfo, levelzero);
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
    eventPoolDesc.count = static_cast<uint32_t>(arguments.numberOfThreads);
    ze_event_pool_handle_t eventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    std::vector<ThreadSpecificData> threadData(arguments.numberOfThreads);
    const size_t bufferSize = 4096u;

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("ulls_benchmark_write_one.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "write_one_uncached";

    for (auto i = 0u; i < arguments.numberOfThreads; i++) {

        commandQueueDesc.ordinal = supportedEngineInfo[i % numberOfSupportedEngines].ordinal;
        commandQueueDesc.index = supportedEngineInfo[i % numberOfSupportedEngines].engineIndex;
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, bufferSize, &threadData[i].hostMemory));
        ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, threadData[i].hostMemory, bufferSize));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &threadData[i].cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &threadData[i].kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(threadData[i].kernel, 1, 1, 1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(threadData[i].kernel, 0, sizeof(threadData[i].hostMemory), &threadData[i].hostMemory));
        eventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &threadData[i].event));
    }

    std::shared_mutex barrier;
    // Warmup
    for (auto i = 0u; i < 5; i++) {
        std::unique_lock lock(barrier);
        std::vector<std::unique_ptr<std::thread>> threads;
        for (auto j = 0u; j < arguments.numberOfThreads; ++j) {
            threads.push_back(std::unique_ptr<std::thread>(
                new std::thread(issueToImmediateCmdList, &threadData[j], &barrier)));
        }
        lock.unlock();
        for (auto j = 0u; j < arguments.numberOfThreads; ++j) {
            threads[j]->join();
        }

        for (auto j = 0u; j < arguments.numberOfThreads; ++j) {
            zeEventHostReset(threadData[j].event);
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
        ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, threadData[i].hostMemory, bufferSize));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(threadData[i].kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, threadData[i].hostMemory));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(threadData[i].cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(threadData[i].event));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ImmediateCommandListSubmission> registerTestCase(run, Api::L0);
