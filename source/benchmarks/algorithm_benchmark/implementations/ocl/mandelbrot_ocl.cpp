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

#include "definitions/mandelbrot.h"

using namespace OCL;

static TestResult run(const MandelbrotArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Opencl opencl(QueueProperties::create().setOoq(false));
    cl_int retVal{};

    const size_t pixelCount = static_cast<size_t>(Mandelbrot::width) * Mandelbrot::height;
    const size_t imageSize = pixelCount * sizeof(float);
    std::vector<float> image(pixelCount);

    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "algorithm_benchmark_mandelbrot.cl", nullptr, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "mandelbrot", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_mem imageBuffer = clCreateBuffer(opencl.context, CL_MEM_WRITE_ONLY, imageSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const cl_int width = static_cast<cl_int>(Mandelbrot::width);
    const cl_int height = static_cast<cl_int>(Mandelbrot::height);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(imageBuffer), &imageBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(width), &width));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(height), &height));

    const size_t globalSize[2] = {Mandelbrot::width, Mandelbrot::height};
    const size_t localSize[2] = {Mandelbrot::groupSize, Mandelbrot::groupSize};
    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto launch = 0u; launch < Mandelbrot::kernelLaunchCount; launch++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 2, nullptr, globalSize, localSize, 0, nullptr, nullptr));
        }
        ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, imageBuffer, CL_TRUE, 0, imageSize, image.data(), 0, nullptr, nullptr));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(imageBuffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<Mandelbrot> registerTestCase(run, Api::OpenCL);
