/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/ioq_kernel_switch_latency.h"

#include <gtest/gtest.h>

static TestResult run(const IoqKernelSwitchLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    queueProperties.setOoq(false);

    Opencl opencl(queueProperties);

    Timer timer;
    cl_int retVal{};
    const size_t gws = 64 * 1024u;
    const size_t lws = 64u;

    // Create buffer
    const auto bufferSize = sizeof(cl_int) * gws;
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Create kernel
    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile("benchmark_write_multiple.cl");
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "write_multiple", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup, kernel
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(retVal);

    cl_ulong start{}, end{};
    std::vector<cl_event> profilingEvents;

    profilingEvents.resize(arguments.kernelCount);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; ++i) {
        timer.measureStart();
        for (auto j = 0u; j < arguments.kernelCount; ++j) {
            if (arguments.useEvents && j >= 1) {
                ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 1, &profilingEvents[j - 1], &profilingEvents[j]));
            } else {
                ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &profilingEvents[j]));
            }
        }

        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        auto switchTime = std::chrono::nanoseconds(0u);
        size_t validSwitchTimeValueCount = 0u;
        for (auto j = 1u; j < arguments.kernelCount; ++j) {
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvents[j - 1], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, nullptr));
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvents[j], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, nullptr));
            auto currentSwitchTime = start - end;
            if (start >= end) {
                switchTime += std::chrono::nanoseconds(currentSwitchTime);
                ++validSwitchTimeValueCount;
            } // disregard invalid values (where kernel start is before previous kernel end)
        }

        if (0 == validSwitchTimeValueCount) {
            return TestResult::Error;
        }

        statistics.pushValue(switchTime / validSwitchTimeValueCount, typeSelector.getUnit(), typeSelector.getType());

        for (auto j = 0u; j < arguments.kernelCount; ++j) {
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvents[j]));
        }
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<IoqKernelSwitchLatency> registerTestCase(run, Api::OpenCL);
