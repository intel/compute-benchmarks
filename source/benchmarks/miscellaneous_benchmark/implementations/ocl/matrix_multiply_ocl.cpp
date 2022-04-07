/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/matrix_multiply.h"

static TestResult run(const MatrixMultiplyArguments &arguments, Statistics &statistics) {
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    Opencl opencl(queueProperties);
    cl_int retVal;

    // Create kernel
    std::string kernelSource = R"(
       __kernel void sum(__global uint *results , __global uint * vectorA , __global uint* vectorB) {
            const int index = get_global_id(2) * get_global_size(0)  * get_global_size(1) + get_global_id(1) * get_global_size(0) + get_global_id(0);
            results[index] = vectorA[index] + vectorB[index];
       }
    )";

    const char *pKernelSource = kernelSource.c_str();
    const size_t kernelSizes = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &pKernelSource, &kernelSizes, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "sum", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Prepare data
    const size_t sizeInElements = arguments.numberOfElementsX * arguments.numberOfElementsY * arguments.numberOfElementsZ;
    const size_t sizeInBytes = sizeInElements * sizeof(int);

    auto dataX = std::make_unique<int[]>(sizeInElements);
    auto dataY = std::make_unique<int[]>(sizeInElements);
    auto results = std::make_unique<int[]>(sizeInElements);

    const size_t gws[] = {arguments.numberOfElementsX, arguments.numberOfElementsY, arguments.numberOfElementsZ};

    int counter = 0u;

    for (auto z = 0u; z < gws[2]; z++) {
        for (auto y = 0u; y < gws[1]; y++) {
            for (auto x = 0u; x < gws[0]; x++) {
                auto index = x + y * gws[0] + z * gws[0] * gws[1];
                dataX[index] = counter++;
                dataY[index] = counter++;
                results[index] = dataX[index] + dataY[index];
            }
        }
    }

    // Create buffer
    cl_mem bufferX = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeInBytes, dataX.get(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_mem bufferY = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeInBytes, dataY.get(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_mem bufferResults = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, sizeInBytes, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Validate results
    cl_event profilingEvent{};
    cl_ulong timeNs{};

    // Warmup kernel
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(bufferResults), &bufferResults));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(bufferX), &bufferX));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(bufferY), &bufferY));

    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 3, nullptr, gws, nullptr, 0, nullptr, &profilingEvent));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    auto resultsFromRun = std::make_unique<int[]>(sizeInElements);

    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, bufferResults, true, 0u, sizeInBytes, resultsFromRun.get(), 0u, nullptr, nullptr));

    if (memcmp(resultsFromRun.get(), results.get(), sizeInBytes) != 0) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferX));
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferY));
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferResults));
        ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
        ASSERT_CL_SUCCESS(clReleaseProgram(program));
        return TestResult::VerificationFail;
    }

    ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
    ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));

    // warmup
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 3, nullptr, gws, nullptr, 0, nullptr, &profilingEvent));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 3, nullptr, gws, nullptr, 0, nullptr, &profilingEvent));
        ASSERT_CL_SUCCESS(clWaitForEvents(1, &profilingEvent));
        ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
        statistics.pushValue(std::chrono::nanoseconds{timeNs}, sizeInBytes * 3, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu, "bw");
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(bufferX));
    ASSERT_CL_SUCCESS(clReleaseMemObject(bufferY));
    ASSERT_CL_SUCCESS(clReleaseMemObject(bufferResults));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MatrixMultiply> registerTestCase(run, Api::OpenCL);
