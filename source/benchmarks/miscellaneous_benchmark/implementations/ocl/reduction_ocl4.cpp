/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/reduction4.h"

TestResult run(const ReductionArguments4 &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    Opencl opencl(queueProperties);
    cl_int retVal;

    // Create kernel
    std::string kernelSource = R"(
       __kernel void reduction(__global uint4 *results, __global uint* partialSums) {
            uint4 values = results[get_global_id(0)];
            uint value = values.x + values.y + values.z + values.w;
            local uint localSums[512u];
            localSums[get_local_id(0)] = value;
            barrier(CLK_LOCAL_MEM_FENCE);
            if(get_local_id(0)==0){
                uint sum = 0u;
                for(int i = 0 ; i < 512u ; i++ ){
                    sum += localSums[i];
                }
                partialSums[get_group_id(0)] = sum;
            }
       }

       __kernel void reduction2(__global uint4 *results, __global uint* partialSums, uint count) {
        uint finalSum = 0;
        for(int i=0 ; i< count ; i++){
            finalSum += partialSums[i];
        }
        partialSums[0] = finalSum;}
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
    cl_kernel finalKernel = clCreateKernel(program, "reduction2", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Prepare data
    const size_t sizeInBytes = (arguments.numberOfElements + 1) * sizeof(int32_t);

    size_t maxAllocSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(maxAllocSize), &maxAllocSize, nullptr);

    if (sizeInBytes > maxAllocSize) {
        return TestResult::DeviceNotCapable;
    }

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
    cl_event profilingEvent2{};
    cl_ulong timeNs{};
    cl_ulong timeNs2{};

    // Warmup kernel
    const size_t gws = arguments.numberOfElements / sizeof(int);
    const size_t lws = 256u;
    const cl_uint groupCount = static_cast<cl_uint>(gws / lws);
    const size_t dispatchOne = 1u;

    cl_mem partialSums = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, groupCount * sizeof(int32_t), nullptr, &retVal);

    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(buffer), &partialSums));

    ASSERT_CL_SUCCESS(clSetKernelArg(finalKernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(finalKernel, 1, sizeof(buffer), &partialSums));
    ASSERT_CL_SUCCESS(clSetKernelArg(finalKernel, 2, sizeof(cl_uint), &groupCount));

    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &profilingEvent));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, finalKernel, 1, nullptr, &dispatchOne, &dispatchOne, 0, nullptr, &profilingEvent2));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, partialSums, true, 0u, 4u, &actualSum, 0u, nullptr, nullptr));
    if (actualSum != expectedSum) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
        ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
        ASSERT_CL_SUCCESS(clReleaseProgram(program));
        return TestResult::VerificationFail;
    }
    ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
    ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent2));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &profilingEvent));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, finalKernel, 1, nullptr, &dispatchOne, &dispatchOne, 0, nullptr, &profilingEvent2));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
        ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent2, timeNs2));
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent2));
        statistics.pushValue(std::chrono::nanoseconds{timeNs}, typeSelector.getUnit(), typeSelector.getType(), "time");
        statistics.pushValue(std::chrono::nanoseconds{timeNs}, sizeInBytes, MeasurementUnit::GigabytesPerSecond, typeSelector.getType(), "bw");
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseMemObject(partialSums));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseKernel(finalKernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<Reduction4> registerTestCase(run, Api::OpenCL);
