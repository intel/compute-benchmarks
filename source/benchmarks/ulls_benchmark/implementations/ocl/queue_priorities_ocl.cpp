/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/queue_priorities.h"

#include <chrono>
#include <gtest/gtest.h>
#include <thread>

static TestResult run(const QueuePrioritiesArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties);
    Timer timer{};
    cl_int retVal{};

    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile("ulls_benchmark_eat_time.cl");
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel lowPriorityKernel = clCreateKernel(program, "eat_time", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_kernel highPriorityKernel = clCreateKernel(program, "eat_time", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // setup kernel time
    cl_uint kernelTime = static_cast<cl_uint>(arguments.lowPriorityKernelTime) * 7u;
    ASSERT_CL_SUCCESS(clSetKernelArg(lowPriorityKernel, 0, sizeof(kernelTime), &kernelTime));
    kernelTime = static_cast<cl_uint>(arguments.highPriorityKernelTime) * 7u;
    ASSERT_CL_SUCCESS(clSetKernelArg(highPriorityKernel, 0, sizeof(kernelTime), &kernelTime));

    cl_command_queue lowPriorityQueue;
    cl_command_queue highPriorityQueue;
    if (arguments.usePriorities) {
        cl_queue_properties lowPriorityProperties[] = {
            CL_QUEUE_FAMILY_INTEL, 0u,
            CL_QUEUE_INDEX_INTEL, 0,
            CL_QUEUE_PRIORITY_KHR, CL_QUEUE_PRIORITY_LOW_KHR,
            0};

        cl_queue_properties highPriorityProperties[] = {
            CL_QUEUE_FAMILY_INTEL, 0u,
            CL_QUEUE_INDEX_INTEL, 1,
            CL_QUEUE_PRIORITY_KHR, CL_QUEUE_PRIORITY_HIGH_KHR,
            0};

        lowPriorityQueue = clCreateCommandQueueWithProperties(opencl.context, opencl.device, lowPriorityProperties, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        highPriorityQueue = clCreateCommandQueueWithProperties(opencl.context, opencl.device, highPriorityProperties, &retVal);
        ASSERT_CL_SUCCESS(retVal);
    } else {
        cl_queue_properties lowPriorityProperties[] = {
            CL_QUEUE_FAMILY_INTEL, 0u,
            CL_QUEUE_INDEX_INTEL, 0,
            0};

        cl_queue_properties highPriorityProperties[] = {
            CL_QUEUE_FAMILY_INTEL, 0u,
            CL_QUEUE_INDEX_INTEL, 1,
            0};

        lowPriorityQueue = clCreateCommandQueueWithProperties(opencl.context, opencl.device, lowPriorityProperties, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        highPriorityQueue = clCreateCommandQueueWithProperties(opencl.context, opencl.device, highPriorityProperties, &retVal);
        ASSERT_CL_SUCCESS(retVal);
    }

    // warmup
    size_t gws = 64u;
    size_t lws = 64u;
    // Warmup, kernel
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(lowPriorityQueue, lowPriorityKernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(lowPriorityQueue));
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(highPriorityQueue, highPriorityKernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(highPriorityQueue));
    ASSERT_CL_SUCCESS(retVal);
    std::this_thread::sleep_for(std::chrono::milliseconds(arguments.sleepTime));

    // benchmark
    size_t gwsLowPriority = 64 * 1024 * 1024;
    size_t gwsHighPriority = arguments.workgroupCount * 64u;
    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(lowPriorityQueue, lowPriorityKernel, 1, nullptr, &gwsLowPriority, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFlush(lowPriorityQueue));

        std::this_thread::sleep_for(std::chrono::milliseconds(arguments.sleepTime));

        // now submit high priority kernel
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(highPriorityQueue, highPriorityKernel, 1, nullptr, &gwsHighPriority, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(highPriorityQueue));
        timer.measureEnd();
        ASSERT_CL_SUCCESS(retVal);
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
        ASSERT_CL_SUCCESS(clFinish(lowPriorityQueue));
        std::this_thread::sleep_for(std::chrono::milliseconds(arguments.sleepTime));
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseCommandQueue(lowPriorityQueue));
    ASSERT_CL_SUCCESS(clReleaseCommandQueue(highPriorityQueue));

    ASSERT_CL_SUCCESS(clReleaseKernel(lowPriorityKernel));
    ASSERT_CL_SUCCESS(clReleaseKernel(highPriorityKernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<QueuePriorities> registerTestCase(run, Api::OpenCL);
