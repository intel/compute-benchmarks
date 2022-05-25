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

#include "definitions/resource_reassign.h"

#include <array>
#include <gtest/gtest.h>
#include <thread>

static TestResult run(const ResourceReassignArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties);
    Timer timer{};
    cl_int retVal{};

    std::array<cl_command_queue, 4> queues{};
    for (size_t i = 0; i < arguments.queueCount + 1; i++) {
        queues[i] = opencl.createQueue(QueueProperties::create().setForceEngine(static_cast<Engine>(static_cast<uint32_t>(Engine::Ccs0) + i)));
    }

    size_t gws = 268435456; //2048 * (512 * 8 * 32)
    size_t size = gws * sizeof(int);
    auto buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, size, nullptr, &retVal);

    const char *source = "__kernel void stress(__global int *buffer) { "
                         "    const int gid = get_global_id(0);"
                         "    buffer[gid] = 1;"
                         "    for (int i = 0; i < 100; i++) {"
                         "        for (int j = 0; j < i; j++) {"
                         "            buffer[gid] *= i + j;"
                         "        }"
                         "    }"
                         "}";

    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel stress = clCreateKernel(program, "stress", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArg(stress, 0, sizeof(buffer), &buffer));

    // Warmup
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(queues[0]));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(queues[0]));
    for (size_t i = 1; i < arguments.queueCount + 1; i++) {
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[i], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queues[i]));
    }

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queues[0]));

        timer.measureStart();

        for (size_t i = 0; i < arguments.queueCount; i++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clFinish(queues[0]));
        }

        timer.measureEnd();
        auto singleQueueDiff = timer.get();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queues[0]));

        timer.measureStart();

        for (size_t i = 1; i < arguments.queueCount + 1; i++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[i], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clFinish(queues[i]));
        }

        timer.measureEnd();
        auto doubleQueueDiff = timer.get();

        auto value = doubleQueueDiff - singleQueueDiff;

        statistics.pushValue(value, MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    for (size_t i = 0; i < arguments.queueCount + 1; i++) {
        ASSERT_CL_SUCCESS(clReleaseCommandQueue(queues[i]));
    }
    ASSERT_CL_SUCCESS(clReleaseKernel(stress));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ResourceReassign> registerTestCase(run, Api::OpenCL);
