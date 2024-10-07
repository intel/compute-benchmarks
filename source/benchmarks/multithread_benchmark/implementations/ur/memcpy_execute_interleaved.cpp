/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/memcpy_execute.h"

#include <gtest/gtest.h>
#include <mutex>
#include <shared_mutex>
#include <thread>

static constexpr size_t global_offset = 0;
static constexpr size_t n_dimensions = 1;

static TestResult run(const MemcpyExecuteArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    bool inOrderQueue = arguments.inOrderQueue;
    bool measureCompletionTime = arguments.measureCompletionTime;
    int numQueuesPerThread = arguments.numQueuesPerThread;
    size_t numThreads = arguments.numThreads;
    size_t allocSize = arguments.allocSize;
    bool useEvents = arguments.useEvents;

    if (!useEvents && !inOrderQueue) {
        std::cerr << "In order queue must be used when events are not used" << std::endl;
        return TestResult::Error;
    }

    // Setup
    UrState ur;
    Timer timer;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("memory_benchmark_fill_with_ones.spv");
    if (spirvModule.size() == 0)
        return TestResult::KernelNotFound;

    ur_program_handle_t program;
    EXPECT_UR_RESULT_SUCCESS(urProgramCreateWithIL(ur.context, spirvModule.data(),
                                                   spirvModule.size(), nullptr, &program));

    EXPECT_UR_RESULT_SUCCESS(urProgramBuild(ur.context, program, nullptr));

    const char *kernelName = "fill_with_ones";

    std::vector<std::vector<void *>> usm(numThreads);
    std::vector<std::vector<ur_kernel_handle_t>> kernels(numThreads);
    std::vector<std::vector<ur_queue_handle_t>> queues(numThreads);
    std::vector<int> src_buffer(allocSize / sizeof(int), 99);
    std::vector<std::vector<std::vector<int>>> dst_buffers(numThreads);

    ur_queue_handle_t singleQueue;

    ur_queue_properties_t queueProperties = {};

    if (!inOrderQueue) {
        queueProperties.flags = UR_QUEUE_FLAG_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }

    // Setup queues (or a single queue if numQueuesPerThread == 0)
    if (numQueuesPerThread == 0) {
        EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                               &queueProperties, &singleQueue));
        for (size_t i = 0; i < numThreads; i++) {
            queues[i].push_back(singleQueue);
        }
    } else {
        for (size_t i = 0; i < numThreads; i++) {
            for (int j = 0; j < numQueuesPerThread; j++) {
                ur_queue_handle_t queue;
                EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                                       &queueProperties, &queue));
                queues[i].push_back(queue);
            }
        }
    }

    // Setup kernels and USM allocations
    for (size_t i = 0; i < numThreads; i++) {
        for (size_t j = 0; j < queues[i].size(); j++) {
            void *ptr;
            EXPECT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, allocSize, &ptr));
            usm[i].push_back(ptr);

            ur_kernel_handle_t kernel;
            EXPECT_UR_RESULT_SUCCESS(urKernelCreate(program, kernelName, &kernel));
            EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernel, 0, nullptr, usm[i][j]));
            kernels[i].push_back(kernel);

            dst_buffers[i].emplace_back(allocSize / sizeof(int), 0);
        }
    }

    auto worker = [&](size_t thread_id, Timer &timer) {
        std::vector<std::vector<ur_event_handle_t>> events(queues[thread_id].size());
        for (auto &events_vec : events) {
            events_vec.assign(3, nullptr);
        }

        timer.measureStart();

        for (size_t i = 0; i < queues[thread_id].size(); i++) {
            auto queue = queues[thread_id][i];
            auto kernel = kernels[thread_id][i];
            auto usm_ptr = usm[thread_id][i];
            auto host_dst = dst_buffers[thread_id][i].data();

            ur_event_handle_t *memcpySignalEventPtr = useEvents ? &events[i][0] : nullptr;
            ur_event_handle_t *kernelSignalEventPtr = useEvents ? &events[i][1] : nullptr;
            ur_event_handle_t *finalSignalEventPtr = useEvents ? &events[i][2] : nullptr;

            EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, usm_ptr, src_buffer.data(), allocSize, 0, nullptr, memcpySignalEventPtr));
            EXPECT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(queue, kernel, n_dimensions, &global_offset, &allocSize, nullptr, useEvents, memcpySignalEventPtr, kernelSignalEventPtr));
            EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, host_dst, usm_ptr, allocSize, useEvents, kernelSignalEventPtr, finalSignalEventPtr));
        }

        if (!measureCompletionTime)
            timer.measureEnd();

        if (useEvents) {
            for (size_t i = 0; i < queues[thread_id].size(); i++) {
                EXPECT_UR_RESULT_SUCCESS(urEventWait(1, &events[i].back()));
            }
        } else {
            for (size_t i = 0; i < queues[thread_id].size(); i++) {
                EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queues[thread_id][i]));
            }
        }

        if (measureCompletionTime)
            timer.measureEnd();

        for (auto &events_vec : events) {
            for (auto &event : events_vec) {
                if (event != nullptr)
                    EXPECT_UR_RESULT_SUCCESS(urEventRelease(event));
            }
        }
    };

    // warmup
    for (auto iteration = 0u; iteration < arguments.numThreads; iteration++) {
        std::vector<std::thread> threads;
        for (size_t j = 0u; j < arguments.numThreads; j++) {
            threads.emplace_back([&, j] {
                Timer dummyTimer;
                worker(j, dummyTimer);
            });
        }
        for (auto &thread : threads) {
            thread.join();
        }
    }

    // Benchmark
    for (size_t i = 0u; i < arguments.iterations; i++) {
        std::shared_mutex barrier;
        std::vector<std::thread> threads;
        std::vector<Timer> timers(arguments.numThreads);

        std::unique_lock<std::shared_mutex> lock(barrier);
        for (size_t j = 0u; j < arguments.numThreads; j++) {
            threads.emplace_back([&, j] {
                std::shared_lock<std::shared_mutex> lock(barrier);
                worker(j, timers[j]);
            });
        }
        lock.unlock();

        auto aggregatedTime = std::chrono::high_resolution_clock::duration(0);
        for (size_t j = 0u; j < arguments.numThreads; j++) {
            threads[j].join();
            aggregatedTime += timers[j].get();
        }
        auto avgTime = aggregatedTime / arguments.numThreads;

#ifndef NDEBUG
        // verify the results
        for (size_t t = 0; t < numThreads; t++) {
            for (size_t i = 0; i < queues[t].size(); i++) {
                for (size_t j = 0; j < allocSize / sizeof(int); j++) {
                    if (dst_buffers[t][i][j] != 1) {
                        return TestResult::Error;
                    }
                }
            }
        }
#endif

        statistics.pushValue(avgTime, typeSelector.getUnit(), typeSelector.getType());
    }

    for (size_t i = 0; i < numThreads; i++) {
        for (size_t j = 0; j < queues[i].size(); j++) {
            EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernels[i][j]));
            EXPECT_UR_RESULT_SUCCESS(urUSMFree(ur.context, usm[i][j]));
            if (numQueuesPerThread != 0) {
                EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queues[i][j]));
            }
        }
    }
    if (numQueuesPerThread == 0) {
        EXPECT_UR_RESULT_SUCCESS(urQueueRelease(singleQueue));
    }
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<MemcpyExecute> registerTestCase(run, Api::UR);
