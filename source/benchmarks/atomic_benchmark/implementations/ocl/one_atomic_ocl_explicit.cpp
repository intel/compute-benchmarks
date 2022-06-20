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

#include "definitions/one_atomic_explicit.h"
#include "kernel_helper.h"

#include <cstring>
#include <gtest/gtest.h>

static TestResult run(const OneAtomicExplicitArguments &arguments, Statistics &statistics) {
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

    // Buffer sizes
    const size_t atomicBufferSize = data.sizeOfDataType; // only one atomic value
    const size_t otherArguemtnsBufferEntryCount = 4u;    // we only need 1 value, but storing in multiple can prevent some compiler opts
    const size_t otherArgumentsBufferSize = otherArguemtnsBufferEntryCount * data.sizeOfDataType;

    // Create kernel
    cl_program program = nullptr;
    const char *programName = "atomic_benchmark_kernel.cl";
    const std::string compilerOptions = KernelHelper::getCompilerOptionsExplicit(arguments.dataType, arguments.atomicOperation, arguments.memoryOrder, arguments.scope, otherArguemtnsBufferEntryCount);
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.c_str(), program); result != TestResult::Success) {
        return result;
    }
    cl_kernel initializeKernel = clCreateKernel(program, "initialize", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_kernel kernel = clCreateKernel(program, "one_atomic", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Create and initialize the atomic buffer with kernel (we need to use atomic_init for explicit atomics)
    const size_t gwsForInitialize = 1u;
    cl_mem atomicBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, atomicBufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArg(initializeKernel, 0, sizeof(atomicBuffer), &atomicBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(initializeKernel, 1, data.sizeOfDataType, data.initialValue));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, initializeKernel, 1, nullptr, &gwsForInitialize, nullptr, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Create and initialize the buffer with value for the other argument of atomic operation
    cl_mem otherArgumentsBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, otherArgumentsBufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, otherArgumentsBuffer, data.otherArgument, data.sizeOfDataType, 0, otherArgumentsBufferSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Warmup
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(atomicBuffer), &atomicBuffer));
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
        auto totalAtomicOperations = data.loopIterations * data.operatorApplicationsPerIteration;
        auto timePerAtomicOperation = timer.get() / totalAtomicOperations;
        statistics.pushValue(timePerAtomicOperation, MeasurementUnit::Nanoseconds, MeasurementType::Cpu);
    }

    // Verify
    std::byte result[8] = {};
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, atomicBuffer, CL_BLOCKING, 0, data.sizeOfDataType, result, 0, nullptr, nullptr));
    if (std::memcmp(result, data.expectedValue, data.sizeOfDataType) != 0) {
        return TestResult::VerificationFail;
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(initializeKernel));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(atomicBuffer));
    ASSERT_CL_SUCCESS(clReleaseMemObject(otherArgumentsBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<OneAtomicExplicit> registerTestCase(run, Api::OpenCL);
