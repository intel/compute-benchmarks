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

#include "definitions/one_atomic.h"
#include "kernel_helper.h"

#include <cstring>
#include <gtest/gtest.h>

static TestResult run(const OneAtomicArguments &arguments, Statistics &statistics) {
    // Setup
    Opencl opencl{};
    Timer timer{};
    cl_int retVal{};

    // Check support
    if (!MathOperationHelper::isSupportedAsAtomic(arguments.atomicOperation, arguments.dataType, opencl.getExtensions().isGlobalFloatAtomicsSupported(), false)) {
        return TestResult::DeviceNotCapable;
    }

    // Prepare data
    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const size_t totalThreadsCount = gws * (arguments.iterations + 1);
    const auto data = KernelHelper::getDataForKernel(arguments.dataType, arguments.atomicOperation, totalThreadsCount);

    // Create and initialize the buffer with atomic
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, data.sizeOfDataType, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffer, CL_FALSE, 0, data.sizeOfDataType, data.initialValue, 0, nullptr, nullptr));

    // Create and initialize the buffer with other argument
    const size_t otherArgumentsBufferSize = 32u;
    cl_mem otherArgumentsBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, data.sizeOfDataType * otherArgumentsBufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, otherArgumentsBuffer, data.otherArgument, data.sizeOfDataType, 0, data.sizeOfDataType * otherArgumentsBufferSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Create kernel
    cl_program program = nullptr;
    const char *programName = "atomic_benchmark_kernel.cl";
    const std::string compilerOptions = KernelHelper::getCompilerOptions(arguments.dataType, arguments.atomicOperation, otherArgumentsBufferSize);
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.c_str(), program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "one_atomic", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(otherArgumentsBuffer), &otherArgumentsBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(data.loopIterations), &data.loopIterations));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Verify
    std::byte result[8] = {};
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, data.sizeOfDataType, result, 0, nullptr, nullptr));
    if (std::memcmp(result, data.expectedValue, data.sizeOfDataType) != 0) {
        return TestResult::VerificationFail;
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseMemObject(otherArgumentsBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<OneAtomic> registerTestCase(run, Api::OpenCL);
