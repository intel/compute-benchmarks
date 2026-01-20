/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_switch_latency_fill.h"

#include <gtest/gtest.h>

static TestResult run(const KernelSwitchLatencyFillArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    queueProperties.setOoq(!arguments.inOrder);

    Opencl opencl(queueProperties);

    Timer timer;
    cl_int retVal{};
    const size_t gws = arguments.fillSize / sizeof(uint32_t);
    const size_t lws = 256u;

    // Create kernel
    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile("ulls_benchmark_fill_with_ones.cl");
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "fill_with_ones", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    auto buffer = clCreateBuffer(opencl.context, 0, arguments.fillSize * sizeof(uint32_t), nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));

    cl_ulong start{}, end{};
    std::vector<cl_event> profilingEvents;

    profilingEvents.resize(arguments.kernelCount);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, &profilingEvents[0]));

        for (auto j = 1u; j < arguments.kernelCount; j++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 1, &profilingEvents[j - 1], &profilingEvents[j]));
        }

        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        auto switchTime = std::chrono::nanoseconds(0u);
        cl_ulong maxSwitchTime = 0llu;
        for (auto j = 1u; j < arguments.kernelCount; j++) {
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvents[j - 1], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, nullptr));
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvents[j], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, nullptr));

            auto currentSwitchTime = start - end;

            if (start < end) {
                currentSwitchTime = maxSwitchTime;
            }

            switchTime += std::chrono::nanoseconds(currentSwitchTime);
            maxSwitchTime = std::max(currentSwitchTime, maxSwitchTime);
        }

        statistics.pushValue(switchTime / (arguments.kernelCount - 1), typeSelector.getUnit(), typeSelector.getType());

        for (auto j = 0u; j < arguments.kernelCount; j++) {
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvents[j]));
        }
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelSwitchLatencyFill> registerTestCase(run, Api::OpenCL);
