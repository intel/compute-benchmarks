/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/best_walker_submission.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

static TestResult run(const BestWalkerSubmissionArguments &arguments, Statistics &statistics) {
    // Setup
    Opencl opencl;
    Timer timer;
    auto clHostMemAllocINTEL = (pfn_clHostMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clHostMemAllocINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clMemFreeINTEL");
    if (!clHostMemAllocINTEL || !clMemFreeINTEL) {
        return TestResult::DriverFunctionNotFound;
    }
    cl_int retVal;

    // Create system memory buffer
    void *hostMemory = clHostMemAllocINTEL(opencl.context, nullptr, 64, 0, &retVal);
    volatile cl_int *volatileHostMemory = static_cast<cl_int *>(hostMemory);
    ASSERT_CL_SUCCESS(retVal);

    // Create kernel
    const char *source = "__kernel void write(__global uint *outBuffer) {  \n"
                         "   outBuffer[0u] = 1;             \n"
                         "}";
    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "write", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Benchmark
    ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 0, hostMemory));
    const size_t gws = 1;
    const size_t lws = 1;
    for (auto i = 0u; i < arguments.iterations; i++) {
        // Warmup, kernel
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        ASSERT_CL_SUCCESS(retVal);

        // Reset value
        *volatileHostMemory = 0;
        _mm_clflush(hostMemory);

        // Enqueue write on GPU and poll for update on CPU
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFlush(opencl.commandQueue));
        ASSERT_CL_SUCCESS(retVal);

        while (*volatileHostMemory != 1) {
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, hostMemory));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerSubmission> registerTestCase(run, Api::OpenCL, true);
