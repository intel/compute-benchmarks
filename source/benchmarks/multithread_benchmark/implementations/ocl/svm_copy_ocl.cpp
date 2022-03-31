/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/svm_copy.h"

#include <gtest/gtest.h>
#include <mutex>
#include <shared_mutex>
#include <thread>

void enqueueSvmCopy(cl_command_queue queue, void *src, void *dst, size_t size, std::shared_mutex *barrier, pfn_clEnqueueMemcpyINTEL clEnqueueMemcpyINTEL) {
    // warmup
    clEnqueueMemcpyINTEL(queue, CL_FALSE, dst, src, size, 0, nullptr, nullptr);
    clFinish(queue);

    std::shared_lock sharedLock(*barrier);
    // benchmark
    clEnqueueMemcpyINTEL(queue, CL_FALSE, dst, src, size, 0, nullptr, nullptr);
    clFinish(queue);
}

static TestResult run(const SvmCopyArguments &arguments, Statistics &statistics) {
    // Setup
    Opencl opencl;
    Timer timer{};
    auto clEnqueueMemcpyINTEL = (pfn_clEnqueueMemcpyINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemcpyINTEL");
    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }

    // Create queues
    std::vector<cl_command_queue> commandQueues;
    QueueProperties queueProperties = QueueProperties::create();
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        // Try to use engines BCS1 to BCS8
        Engine copyEngine = EngineHelper::getBlitterEngineFromIndex((i + 1) % 9);
        queueProperties.setForceEngine(copyEngine);
        cl_command_queue queue = opencl.createQueue(queueProperties);
        if (queue) {
            commandQueues.push_back(queue);
        }
    }
    // If no copy engines available, use default queue
    if (commandQueues.empty()) {
        commandQueues.push_back(opencl.commandQueue);
    }

    // Create buffers
    const size_t bufferForCopySize = 64 * 1024;
    std::vector<UsmHelperOcl::Alloc> srcAllocs;
    std::vector<UsmHelperOcl::Alloc> dstAllocs;
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        UsmHelperOcl::Alloc srcAlloc{};
        ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Host, bufferForCopySize, srcAlloc));
        srcAllocs.push_back(srcAlloc);

        UsmHelperOcl::Alloc dstAlloc{};
        ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Device, bufferForCopySize, dstAlloc));
        dstAllocs.push_back(dstAlloc);
    }

    std::shared_mutex barrier;
    // Warmup
    {
        std::unique_lock lock(barrier);
        std::vector<std::unique_ptr<std::thread>> threads;
        for (auto i = 0u; i < arguments.numberOfThreads; i++) {
            threads.push_back(std::unique_ptr<std::thread>(
                new std::thread(enqueueSvmCopy, commandQueues[i % commandQueues.size()], srcAllocs[i].ptr, dstAllocs[i].ptr, bufferForCopySize, &barrier, clEnqueueMemcpyINTEL)));
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
                new std::thread(enqueueSvmCopy, commandQueues[j % commandQueues.size()], srcAllocs[j].ptr, dstAllocs[j].ptr, bufferForCopySize, &barrier, clEnqueueMemcpyINTEL)));
        }
        timer.measureStart();
        lock.unlock();
        for (auto j = 0u; j < arguments.numberOfThreads; j++) {
            threads[j]->join();
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    for (auto i = 0u; i < arguments.numberOfThreads; i++) {
        ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(srcAllocs[i]));
        ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(dstAllocs[i]));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<SvmCopy> registerTestCase(run, Api::OpenCL);
