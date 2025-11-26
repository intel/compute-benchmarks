/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/queue_families_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/sleep.h"
#include "framework/utility/timer.h"

#include "definitions/queue_concurrency.h"

#include <chrono>
#include <gtest/gtest.h>

static TestResult run(const QueueConcurrencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Opencl opencl;
    Timer timer{};
    cl_int retVal{};

    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile("ulls_benchmark_eat_time.cl");
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel slowKernel = clCreateKernel(program, "eat_time", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_kernel fastKernel = clCreateKernel(program, "eat_time", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // setup kernel time
    cl_uint kernelTime = static_cast<cl_uint>(arguments.kernelTime) * 7u;
    ASSERT_CL_SUCCESS(clSetKernelArg(slowKernel, 0, sizeof(kernelTime), &kernelTime));
    cl_uint fastTime = 1u;
    ASSERT_CL_SUCCESS(clSetKernelArg(fastKernel, 0, sizeof(fastTime), &fastTime));

    // warmup
    size_t lws = 32u;
    size_t gws = lws * arguments.workgroupCount;
    std::vector<cl_event> events;
    events.resize(arguments.kernelCount * 2);

    // Warmup, kernel
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, slowKernel, 1, nullptr, &gws, &lws, 0, nullptr, &events[0]));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, fastKernel, 1, nullptr, &gws, &lws, 0, nullptr, &events[1]));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    ASSERT_CL_SUCCESS(clReleaseEvent(events[0u]));
    ASSERT_CL_SUCCESS(clReleaseEvent(events[1u]));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, slowKernel, 1, nullptr, &gws, &lws, 0, nullptr, &events[0]));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, fastKernel, 1, nullptr, &gws, &lws, 0, nullptr, &events[1]));

        for (auto j = 2u; j < arguments.kernelCount * 2; j += 2) {
            // wait only for fast kernel
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, slowKernel, 1, nullptr, &gws, &lws, 1, &events[j - 1], &events[j]));
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, fastKernel, 1, nullptr, &gws, &lws, 0, nullptr, &events[j + 1]));
        }
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        ASSERT_CL_SUCCESS(retVal);
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        for (auto j = 0u; j < arguments.kernelCount * 2; j++) {
            ASSERT_CL_SUCCESS(clReleaseEvent(events[j]));
        }
    }

    // Cleanup

    ASSERT_CL_SUCCESS(clReleaseKernel(slowKernel));
    ASSERT_CL_SUCCESS(clReleaseKernel(fastKernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<QueueConcurrency> registerTestCase(run, Api::OpenCL);
