/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/enum/engine.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/svm_copy.h"

#include <gtest/gtest.h>
#include <mutex>
#include <shared_mutex>
#include <thread>

void enqueueSvmCopy(ze_command_queue_handle_t queue, ze_command_list_handle_t cmdList, std::shared_mutex *barrier) {
    std::shared_lock sharedLock(*barrier);
    zeCommandQueueExecuteCommandLists(queue, 1, &cmdList, nullptr);
    zeCommandQueueSynchronize(queue, std::numeric_limits<uint64_t>::max());
}

static TestResult run(const SvmCopyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer{};

    // Create queues
    std::vector<ze_command_queue_handle_t> queues;
    std::vector<ze_command_queue_desc_t> queueDescriptors;

    auto queueFamiliesDesc = QueueFamiliesHelper::queryQueueFamilies(levelzero.device);
    for (auto copyEngineIndex = 1u; copyEngineIndex <= 8; copyEngineIndex++) {
        auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, EngineHelper::getBlitterEngineFromIndex(copyEngineIndex));
        if (nullptr != queueDesc) {
            queues.push_back(levelzero.createQueue(levelzero.device, queueDesc->desc));
            queueDescriptors.push_back(queueDesc->desc);
        }
    }

    // If no copy engines available, use default queue
    if (queues.empty()) {
        queues.push_back(levelzero.commandQueue);
        queueDescriptors.push_back(levelzero.commandQueueDesc);
    }

    // Create buffers
    const size_t bufferForCopySize = 1024 * 1024;
    std::vector<void *> srcAllocs;
    std::vector<void *> dstAllocs;
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        void *source{}, *destination{};
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, bufferForCopySize, &source));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, bufferForCopySize, &destination));
        srcAllocs.push_back(source);
        dstAllocs.push_back(destination);
    }

    // Create cmdlists
    std::vector<ze_command_list_handle_t> cmdLists;
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        ze_command_list_desc_t cmdListDesc{};
        cmdListDesc.commandQueueGroupOrdinal = queueDescriptors[i % queueDescriptors.size()].ordinal;
        ze_command_list_handle_t cmdList{};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dstAllocs[i], srcAllocs[i], bufferForCopySize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        cmdLists.push_back(cmdList);
    }

    std::shared_mutex barrier;
    // Warmup
    {
        std::unique_lock lock(barrier);
        std::vector<std::unique_ptr<std::thread>> threads;
        for (auto i = 0u; i < arguments.numberOfThreads; i++) {
            threads.push_back(std::unique_ptr<std::thread>(
                new std::thread(enqueueSvmCopy, queues[i % queues.size()], cmdLists[i], &barrier)));
        }
        lock.unlock();
        for (auto i = 0u; i < arguments.numberOfThreads; i++) {
            threads[i]->join();
        }
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        std::unique_lock lock(barrier);
        std::vector<std::unique_ptr<std::thread>> threads;
        for (auto j = 0u; j < arguments.numberOfThreads; j++) {
            threads.push_back(std::unique_ptr<std::thread>(
                new std::thread(enqueueSvmCopy, queues[j % queues.size()], cmdLists[j], &barrier)));
        }
        timer.measureStart();
        lock.unlock();
        for (auto j = 0u; j < arguments.numberOfThreads; j++) {
            threads[j]->join();
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Host, levelzero, srcAllocs[i]));
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, dstAllocs[i]));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<SvmCopy> registerTestCase(run, Api::L0);
