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

#include "definitions/usm_shared_first_gpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedFirstGpuAccessArguments &arguments, Statistics &statistics) {
    // Setup
    Opencl opencl;
    Timer timer;
    auto clSharedMemAllocINTEL = (pfn_clSharedMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clSharedMemAllocINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clMemFreeINTEL");
    if (!clSharedMemAllocINTEL || !clMemFreeINTEL) {
        return TestResult::DriverFunctionNotFound;
    }
    cl_int retVal{};

    // Create kernel
    const char *source = "__kernel void write(__global uint *outBuffer) {  \n"
                         "   outBuffer[0u] = 1;                            \n"
                         "}";
    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "write", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    const cl_mem_properties_intel properties[] = {
        CL_MEM_ALLOC_FLAGS_INTEL,
        UsmHelperOcl::getInitialPlacementFlag(arguments.initialPlacement),
        0,
    };
    const size_t gws = 1;
    const size_t lws = 1;
    auto buffer = clSharedMemAllocINTEL(opencl.context, opencl.device, properties, arguments.bufferSize, 0u, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 0, buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, buffer));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        buffer = clSharedMemAllocINTEL(opencl.context, opencl.device, properties, arguments.bufferSize, 0u, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 0, buffer));

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFlush(opencl.commandQueue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);

        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, buffer));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedFirstGpuAccess> registerTestCase(run, Api::OpenCL, true);
