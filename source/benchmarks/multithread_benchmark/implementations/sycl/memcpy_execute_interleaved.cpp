/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/memcpy_execute.h"

#include <mutex>
#include <shared_mutex>
#include <thread>

static auto inOrder = sycl::property::queue::in_order();
static const sycl::property_list queueProps[] = {
    sycl::property_list{},
    sycl::property_list{inOrder}};

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
    bool useBarrier = arguments.useBarrier;
    size_t arraySize = allocSize / sizeof(int);

    if (!inOrderQueue) {
        std::cerr << "Out of order mode not supported yet" << std::endl;
        return TestResult::Error;
    }

    // Setup
    Timer timer;

    const size_t gws = arraySize;
    const size_t lws = 1u;
    sycl::nd_range<1> range(gws, lws);

    auto queuePropsIndex = 0;
    queuePropsIndex |= arguments.inOrderQueue ? 0x1 : 0;

    std::vector<std::vector<void *>> usm(numThreads);
    std::vector<sycl::queue> queues;

    // Setup queues (or a single queue if !useQueuePerThread)
    if (!useQueuePerThread) {
        sycl::queue singleQueue{queueProps[queuePropsIndex]};
        for (size_t i = 0; i < numThreads; i++) {
            queues.push_back(singleQueue);
        }
    } else {
        for (size_t i = 0; i < numThreads; i++) {
            queues.emplace_back(queueProps[queuePropsIndex]);
        }
    }

    void *src_buffer;
    std::vector<void *> dst_buffers;

    if (srcUSM) {
        src_buffer = sycl::malloc_host(allocSize, queues[0].get_context());
    } else {
        src_buffer = malloc(allocSize);
    }

    if (src_buffer == nullptr) {
        std::cerr << "Failed to allocate memory for src_buffer" << std::endl;
        return TestResult::Error;
    }

    memset(src_buffer, 99, allocSize);

    // Setup USM allocations
    for (size_t i = 0; i < numThreads; i++) {
        for (size_t j = 0; j < numOpsPerThread; j++) {
            usm[i].push_back(sycl::malloc_device(allocSize, queues[i].get_device(), queues[i].get_context()));
        }

        dst_buffers.emplace_back();

        if (dstUSM) {
            dst_buffers.back() = sycl::malloc_host(allocSize * numOpsPerThread, queues[0].get_context());
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
        timer.measureStart();

        auto &queue = queues[thread_id];
        for (size_t i = 0; i < numOpsPerThread; i++) {
            int *usm_ptr = (int *)usm[thread_id][i];
            auto host_dst = ((char *)dst_buffers[thread_id]) + i * allocSize;

            if (useEvents) {
                queue.memcpy(usm_ptr, src_buffer, allocSize);
                queue.parallel_for(sycl::range<1>{arraySize}, [usm_ptr](sycl::item<1> itemId) {
                    auto id = itemId.get_id(0);
                    usm_ptr[id] = 1;
                });
                queue.memcpy(host_dst, usm_ptr, allocSize);

                if (useBarrier) {
                    queue.ext_oneapi_submit_barrier();
                }
            } else {
                sycl::ext::oneapi::experimental::memcpy(queue, usm_ptr, src_buffer, allocSize);
                sycl::ext::oneapi::experimental::nd_launch(queue, range, [usm_ptr](sycl::nd_item<1> itemId) {
                    auto id = itemId.get_global_id(0);
                    usm_ptr[id] = 1;
                });
                sycl::ext::oneapi::experimental::memcpy(queue, host_dst, usm_ptr, allocSize);

                if (useBarrier) {
                    queue.ext_oneapi_submit_barrier();
                }
            }
        }

        if (!measureCompletionTime)
            timer.measureEnd();

        queue.wait();

        if (measureCompletionTime)
            timer.measureEnd();
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
        sycl::free(src_buffer, queues[0].get_context());
    } else {
        free(src_buffer);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<MemcpyExecute> registerTestCase(run, Api::SYCL);
