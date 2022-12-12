/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_with_work_split.h"

#include <gtest/gtest.h>

#define PROVIDE_PROFLING_DETAILS 0

static TestResult run(const KernelWithWorkArgumentsSplit &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    std::vector<cl_event> profilingEvents(arguments.splitSize);
    Opencl opencl(queueProperties);
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
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, arguments.useEvents ? &profilingEvents[0] : nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(retVal);
    if (arguments.useEvents) {
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvents[0]))
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (uint32_t splitId = 0u; splitId < arguments.splitSize; splitId++) {
            globalWorkOffset = splitId * arguments.workgroupSize * workgorupsInOneSplit;
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, &globalWorkOffset, &gws, &lws, 0, nullptr, arguments.useEvents ? &profilingEvents[splitId] : nullptr));
        }
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        if (arguments.useEvents) {
            cl_ulong timeNs{};
            cl_ulong start{};
            cl_ulong end{};
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvents.front(), CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, nullptr));
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvents.back(), CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, nullptr));
            for (uint32_t splitId = 0u; splitId < arguments.splitSize; splitId++) {
                ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvents[splitId]))
            }
            timeNs = end - start;
            statistics.pushValue(std::chrono::nanoseconds(timeNs), typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelWithWorkSplit> registerTestCase(run, Api::OpenCL);
