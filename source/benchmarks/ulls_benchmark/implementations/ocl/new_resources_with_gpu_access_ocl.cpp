/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/new_resources_with_gpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const NewResourcesWithGpuAccessArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

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

    // Get work size
    const size_t elements = arguments.size / sizeof(cl_uint);
    size_t lws{};
    ASSERT_CL_SUCCESS(clGetKernelWorkGroupInfo(kernel, opencl.device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(lws), &lws, nullptr));
    size_t wgc = elements / lws; // may be rounded down, but that shouldn't matter for big buffers
    if (wgc == 0) {
        // very small buffer
        lws = elements;
        wgc = 1;
    }
    size_t gws = wgc * lws;

    const size_t sizeInBytes = arguments.size;

    // Initial buffer to match previous code path that kept one alive between iterations
    cl_mem previousBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, sizeInBytes, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(previousBuffer), &previousBuffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, sizeInBytes, nullptr, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        ASSERT_CL_SUCCESS(retVal);
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        // Store buffer used in this iteration to avoid reuse
        ASSERT_CL_SUCCESS(clReleaseMemObject(previousBuffer));
        previousBuffer = buffer;
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseMemObject(previousBuffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<NewResourcesWithGpuAccess> registerTestCase(run, Api::OpenCL);
