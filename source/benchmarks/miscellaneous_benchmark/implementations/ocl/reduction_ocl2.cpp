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

#include "definitions/reduction2.h"

TestResult run(const ReductionArguments2 &arguments, Statistics &statistics) {
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    Opencl opencl(queueProperties);
    cl_int retVal;

    // Create kernel
    std::string kernelSource = R"(
       __kernel void reduction(__global uint4 *results) {
            uint4 values = results[get_global_id(0)];
            uint value = values.x + values.y + values.z + values.w;
            local uint localSums[256u];
            localSums[get_local_id(0)] = value;
            barrier(CLK_LOCAL_MEM_FENCE);
            if(get_local_id(0)==0){
                uint sum = 0u;
                for(int i = 0 ; i < 256u ; i++ ){
                    sum += localSums[i];
                }
                atomic_add(&(((global uint*)results)[get_global_size(0)*4]), sum);
            }
       }
    )";
    const char *pKernelSource = kernelSource.c_str();
    const size_t kernelSizes = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &pKernelSource, &kernelSizes, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    retVal = clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr);

    if (retVal) {
        size_t numBytes = 0;
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes));
        auto buffer = std::make_unique<char[]>(numBytes);
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, buffer.get(), &numBytes));
        std::cout << buffer.get() << std::endl;
    }

    cl_kernel kernel = clCreateKernel(program, "reduction", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Prepare data
    const size_t sizeInBytes = (arguments.numberOfElements + 1) * sizeof(int32_t);
    auto lastIndexOffset = arguments.numberOfElements * sizeof(int32_t);
    auto data = std::make_unique<int32_t[]>(arguments.numberOfElements + 1);
    int32_t expectedSum = 0u;
    int32_t value = 0u;
    for (auto i = 0u; i < arguments.numberOfElements; i++) {
        value++;
        if (value > 4)
            value = 0;
        data[i] = static_cast<int32_t>(value);
        expectedSum += value;
    }
    data[arguments.numberOfElements] = 0u;

    // Create buffer
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeInBytes, data.get(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Validate results
    int32_t actualSum;
    cl_event profilingEvent{};
    cl_ulong timeNs{};

    // Warmup kernel
    const size_t gws = arguments.numberOfElements / 4;
    const size_t lws = 256;
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &profilingEvent));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, true, lastIndexOffset, 4u, &actualSum, 0u, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    if (actualSum != expectedSum) {
        printf("\n data verification failure");
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
        ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
        ASSERT_CL_SUCCESS(clReleaseProgram(program));
        return TestResult::VerificationFail;
    }

    ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
    ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
    statistics.pushValue(std::chrono::nanoseconds{timeNs}, MeasurementUnit::Microseconds, MeasurementType::Gpu, "time");
    statistics.pushValue(std::chrono::nanoseconds{timeNs}, sizeInBytes, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu, "bw");

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        int zero = 0u;
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffer, true, lastIndexOffset, 4u, &zero, 0u, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &profilingEvent));
        ASSERT_CL_SUCCESS(clWaitForEvents(1, &profilingEvent));
        ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
        if (i + 1 < arguments.iterations) {
            statistics.pushValue(std::chrono::nanoseconds{timeNs}, MeasurementUnit::Microseconds, MeasurementType::Gpu, "time");
            statistics.pushValue(std::chrono::nanoseconds{timeNs}, sizeInBytes, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu, "bw");
        }
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<Reduction2> registerTestCase(run, Api::OpenCL);
