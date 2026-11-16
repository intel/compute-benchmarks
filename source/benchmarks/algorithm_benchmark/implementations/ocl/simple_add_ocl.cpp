/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/simple_add.h"

using namespace OCL;

static TestResult run(const SimpleAddArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Opencl opencl{};
    cl_int retVal{};

    const size_t bufferSize = SimpleAdd::elementCount * sizeof(cl_int);
    const std::vector<cl_int> input(SimpleAdd::elementCount, 1);
    std::vector<cl_int> output(SimpleAdd::elementCount);

    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "algorithm_benchmark_simple_add.cl", nullptr, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "simple_add", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_mem inputBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_ONLY, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem outputBuffer = clCreateBuffer(opencl.context, CL_MEM_WRITE_ONLY, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const cl_int elementCount = static_cast<cl_int>(SimpleAdd::elementCount);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(elementCount), &elementCount));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(inputBuffer), &inputBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(outputBuffer), &outputBuffer));

    const size_t globalSize = 1u;
    const size_t localSize = 1u;
    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, inputBuffer, CL_FALSE, 0, bufferSize, input.data(), 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalSize, &localSize, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, outputBuffer, CL_TRUE, 0, bufferSize, output.data(), 0, nullptr, nullptr));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(outputBuffer));
    ASSERT_CL_SUCCESS(clReleaseMemObject(inputBuffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<SimpleAdd> registerTestCase(run, Api::OpenCL);
