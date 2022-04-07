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

#include "definitions/multi_queue_submission.h"

#include <gtest/gtest.h>

static TestResult run(const MultiQueueSubmissionArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties);
    Timer timer{};
    cl_int retVal{};

    size_t gws = arguments.workgroupCount * arguments.workgroupSize;
    size_t size = gws * sizeof(int);
    std::vector<cl_command_queue> queues;
    std::vector<cl_mem> buffers;
    for (size_t i = 0; i < arguments.queueCount; i++) {
        queues.push_back(opencl.createQueue(QueueProperties::create()));
        buffers.push_back(clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, size, nullptr, &retVal));
    }

    const char *source = "__kernel void fill_with_ones(__global int *buffer) { "
                         "    const int gid = get_global_id(0);"
                         "    buffer[gid] = 1;"
                         "}";
    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "fill_with_ones", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    for (size_t i = 0; i < arguments.queueCount; i++) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffers[i]), &buffers[i]));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[i], kernel, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
    }
    for (size_t i = 0; i < arguments.queueCount; i++) {
        clFinish(queues[i]);
    }

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {

        timer.measureStart();
        for (size_t i = 0; i < arguments.queueCount; i++) {
            ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffers[i]), &buffers[i]));
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[i], kernel, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clFlush(queues[i]));
        }
        for (size_t i = 0; i < arguments.queueCount; i++) {
            ASSERT_CL_SUCCESS(clFinish(queues[i]));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    for (size_t i = 0; i < arguments.queueCount; i++) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffers[i]));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiQueueSubmission> registerTestCase(run, Api::OpenCL);
