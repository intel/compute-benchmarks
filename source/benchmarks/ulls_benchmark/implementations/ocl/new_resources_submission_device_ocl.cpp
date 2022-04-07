/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/new_resources_submission_device.h"

#include <gtest/gtest.h>

static TestResult run(const NewResourcesSubmissionDeviceArguments &arguments, Statistics &statistics) {
    // Setup
    Opencl opencl;
    Timer timer;
    cl_int retVal;

    // Create kernel
    const char *source = "__kernel void write(__global int *outBuffer) {  \n"
                         "   outBuffer[get_global_id(0)] = 1;             \n"
                         "}";
    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "write", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup kernel
    const size_t gws = 1;
    const size_t lws = 1;
    const size_t sizeInBytes = arguments.size;
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, sizeInBytes, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(retVal);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, sizeInBytes, nullptr, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<NewResourcesSubmissionDevice> registerTestCase(run, Api::OpenCL);
