/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/math_operation_helper.h"
#include "framework/utility/timer.h"

#include "definitions/read_after_atomic_write.h"

#include <cstring>
#include <gtest/gtest.h>

static TestResult run(const ReadAfterAtomicWriteArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Opencl opencl{};
    Timer timer{};
    cl_int retVal{};

    // Prepare data
    const size_t workgroupCount = 128;
    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * workgroupCount;
    const size_t loopIterations = 100u;
    const cl_int initialValue = 0u;
    const cl_int useAtomic = static_cast<bool>(arguments.atomic);
    const cl_int shuffleRead = static_cast<bool>(arguments.shuffleRead);

    // Create and initialize the buffer with test data
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, gws * sizeof(cl_int), nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffer, &initialValue, sizeof(initialValue), 0, gws * sizeof(cl_int), 0, nullptr, nullptr));

    // Create kernel
    cl_program program = nullptr;
    const char *programName = "eu_benchmark_read_after_atomic_write.cl";
    const char *kernelName = "read_after_atomic_write";
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, nullptr, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(useAtomic), &useAtomic));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(shuffleRead), &shuffleRead));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 3, sizeof(loopIterations), &loopIterations));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ReadAfterAtomicWrite> registerTestCase(run, Api::OpenCL);
