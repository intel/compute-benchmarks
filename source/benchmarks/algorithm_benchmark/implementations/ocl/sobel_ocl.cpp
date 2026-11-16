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

#include "definitions/sobel.h"
#include "utility/workload_input.h"

using namespace OCL;

static TestResult run(const SobelArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Opencl opencl{};
    cl_int retVal{};

    const size_t pixelCount = static_cast<size_t>(Sobel::width) * Sobel::height;
    const size_t imageSize = pixelCount * sizeof(uint32_t);
    const std::vector<uint32_t> inputImage = WorkloadInput::generateGrayscaleImage(Sobel::width, Sobel::height);
    std::vector<uint32_t> outputImage(pixelCount);

    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "algorithm_benchmark_sobel.cl", nullptr, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "sobel", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_mem inputBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_ONLY, imageSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem outputBuffer = clCreateBuffer(opencl.context, CL_MEM_WRITE_ONLY, imageSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const cl_int width = static_cast<cl_int>(Sobel::width);
    const cl_int height = static_cast<cl_int>(Sobel::height);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(inputBuffer), &inputBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(outputBuffer), &outputBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(width), &width));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 3, sizeof(height), &height));

    const size_t globalSize[2] = {Sobel::width, Sobel::height};
    const size_t localSize[2] = {Sobel::groupSize, Sobel::groupSize};
    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, inputBuffer, CL_FALSE, 0, imageSize, inputImage.data(), 0, nullptr, nullptr));
        for (auto launch = 0u; launch < Sobel::kernelLaunchCount; launch++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 2, nullptr, globalSize, localSize, 0, nullptr, nullptr));
        }
        ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, outputBuffer, CL_TRUE, 0, imageSize, outputImage.data(), 0, nullptr, nullptr));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(outputBuffer));
    ASSERT_CL_SUCCESS(clReleaseMemObject(inputBuffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<Sobel> registerTestCase(run, Api::OpenCL);
