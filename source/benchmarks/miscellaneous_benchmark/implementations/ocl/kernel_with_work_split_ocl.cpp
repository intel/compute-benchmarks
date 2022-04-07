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

#include "definitions/kernel_with_work_split.h"

#include <gtest/gtest.h>

#define PROVIDE_PROFLING_DETAILS 0

static TestResult run(const KernelWithWorkArgumentsSplit &arguments, Statistics &statistics) {
    // Setup
    Opencl opencl;
    Timer timer;
    cl_int retVal{};

    const size_t workgorupsInOneSplit = arguments.workgroupCount / arguments.splitSize;
    const size_t gws = workgorupsInOneSplit * arguments.workgroupSize;
    const size_t lws = arguments.workgroupSize;
    size_t globalWorkOffset = 0u;

    const auto bufferSize = sizeof(cl_int) * arguments.workgroupCount * arguments.workgroupSize;
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile(selectKernel(arguments.usedIds, "cl"));
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "write_one", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup, kernel
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(retVal);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (uint32_t splitId = 0u; splitId < arguments.splitSize; splitId++) {
            globalWorkOffset = splitId * arguments.workgroupSize * workgorupsInOneSplit;
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, &globalWorkOffset, &gws, &lws, 0, nullptr, nullptr));
        }

        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelWithWorkSplit> registerTestCase(run, Api::OpenCL);
