/*
 * Copyright (C) 2024-2025 Intel Corporation
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
    size_t numOpsPerThread = arguments.numOpsPerThread;
    size_t numThreads = arguments.numThreads;
    size_t allocSize = arguments.allocSize;
    bool useEvents = arguments.useEvents;
    bool useQueuePerThread = arguments.useQueuePerThread;
    bool srcUSM = arguments.srcUSM;
    bool dstUSM = arguments.dstUSM;
    size_t arraySize = allocSize / sizeof(int);
    bool useBarrier = arguments.useBarrier;

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
    std::vector<ur_queue_handle_t> queues(numThreads);

    ur_queue_handle_t singleQueue = nullptr;

    ur_queue_properties_t queueProperties = {};

    if (!inOrderQueue) {
        queueProperties.flags = UR_QUEUE_FLAG_OUT_OF_ORDER_EXEC_MODE_ENABLE;
    }

    // Setup queues (or a single queue if !useQueuePerThread)
    if (!useQueuePerThread) {
        EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                               &queueProperties, &singleQueue));
        for (size_t i = 0; i < numThreads; i++) {
            queues[i] = singleQueue;
        }
    } else {
        for (size_t i = 0; i < numThreads; i++) {
            ur_queue_handle_t queue;
            EXPECT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device,
                                                   &queueProperties, &queue));
            queues[i] = queue;
        }
    }

    void *src_buffer;
    std::vector<void *> dst_buffers;

    if (srcUSM) {
        EXPECT_UR_RESULT_SUCCESS(urUSMHostAlloc(ur.context, nullptr, nullptr, allocSize, &src_buffer));
    } else {
        src_buffer = malloc(allocSize);
    }
    if (src_buffer == nullptr) {
        std::cerr << "Failed to allocate memory for src_buffer" << std::endl;
        return TestResult::Error;
    }

    memset(src_buffer, 99, allocSize);

    // Setup kernels and USM allocations
    for (size_t i = 0; i < numThreads; i++) {
        for (size_t j = 0; j < numOpsPerThread; j++) {
            void *ptr;
            EXPECT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, allocSize, &ptr));
            usm[i].push_back(ptr);

            ur_kernel_handle_t kernel;
            EXPECT_UR_RESULT_SUCCESS(urKernelCreate(program, kernelName, &kernel));
            EXPECT_UR_RESULT_SUCCESS(urKernelSetArgPointer(kernel, 0, nullptr, usm[i][j]));
            kernels[i].push_back(kernel);
        }

        dst_buffers.emplace_back();

        if (dstUSM) {
            EXPECT_UR_RESULT_SUCCESS(urUSMHostAlloc(ur.context, nullptr, nullptr, allocSize * numOpsPerThread, &dst_buffers.back()));
        } else {
            dst_buffers.back() = malloc(allocSize * numOpsPerThread);
        }
        if (dst_buffers.back() == nullptr) {
            std::cerr << "Failed to allocate memory for dst_buffer" << std::endl;
            return TestResult::Error;
        }
        memset(dst_buffers.back(), 0, allocSize * numOpsPerThread);
    }

    auto worker = [&](size_t thread_id, Timer &timer) {
        std::vector<std::vector<ur_event_handle_t>> events(numOpsPerThread);
        for (auto &events_vec : events) {
            events_vec.assign(4, nullptr);
        }

        timer.measureStart();

        auto queue = queues[thread_id];
        for (size_t i = 0; i < numOpsPerThread; i++) {
            auto kernel = kernels[thread_id][i];
            auto usm_ptr = usm[thread_id][i];
            auto host_dst = ((char *)dst_buffers[thread_id]) + i * allocSize;

            ur_event_handle_t *memcpySignalEventPtr = useEvents ? &events[i][0] : nullptr;
            ur_event_handle_t *kernelSignalEventPtr = useEvents ? &events[i][1] : nullptr;
            ur_event_handle_t *finalSignalEventPtr = useEvents ? &events[i][2] : nullptr;

            EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, usm_ptr, src_buffer, allocSize, 0, nullptr, memcpySignalEventPtr));
            EXPECT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(queue, kernel, n_dimensions, &global_offset, &arraySize, nullptr, useEvents, memcpySignalEventPtr, kernelSignalEventPtr));
            EXPECT_UR_RESULT_SUCCESS(urEnqueueUSMMemcpy(queue, false, host_dst, usm_ptr, allocSize, useEvents, kernelSignalEventPtr, finalSignalEventPtr));

            if (useBarrier) {
                EXPECT_UR_RESULT_SUCCESS(urEnqueueEventsWaitWithBarrier(queue, useEvents, finalSignalEventPtr, useEvents ? &events[i][3] : nullptr));
            }
        }

        if (!measureCompletionTime)
            timer.measureEnd();

        if (useEvents) {
            for (size_t i = 0; i < numOpsPerThread; i++) {
                EXPECT_UR_RESULT_SUCCESS(urEventWait(1, &events[i].back()));
            }
        } else {
            EXPECT_UR_RESULT_SUCCESS(urQueueFinish(queue));
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
        auto res = verifyResults(numThreads, numOpsPerThread, allocSize, dst_buffers, 1);
        if (res != TestResult::Success)
            return res;
#endif

        statistics.pushValue(avgTime, typeSelector.getUnit(), typeSelector.getType());
    }

    if (srcUSM) {
        EXPECT_UR_RESULT_SUCCESS(urUSMFree(ur.context, src_buffer));
    } else {
        free(src_buffer);
    }

    for (size_t i = 0; i < numThreads; i++) {
        for (size_t j = 0; j < numOpsPerThread; j++) {
            EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernels[i][j]));
            EXPECT_UR_RESULT_SUCCESS(urUSMFree(ur.context, usm[i][j]));
        }

        if (dstUSM) {
            EXPECT_UR_RESULT_SUCCESS(urUSMFree(ur.context, dst_buffers[i]));
        } else {
            free(dst_buffers[i]);
        }

        if (!singleQueue) {
            EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queues[i]));
        }
    }
    if (singleQueue) {
        EXPECT_UR_RESULT_SUCCESS(urQueueRelease(singleQueue));
    }
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<MemcpyExecute> registerTestCase(run, Api::UR);
