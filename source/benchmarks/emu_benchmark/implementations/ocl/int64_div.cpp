/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/int64_div.h"

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/math_operation_helper.h"
#include "framework/utility/timer.h"

#include <cstring>
#include <gtest/gtest.h>

static TestResult run(const Int64DivArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);
    cl_event profilingEvent{};
    cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;
    Timer timer{};
    cl_int retVal{};

    // Prepare data
    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const cl_long initialValue = std::numeric_limits<int64_t>::max() - 1;
    const cl_long divisor = 2;
    const size_t loopIterations = 640u; // tweakable constant

    // Create and initialize the buffer with test data
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, gws * sizeof(cl_long), nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffer, &initialValue, sizeof(initialValue), 0, gws * sizeof(cl_long), 0, nullptr, nullptr));

    // Create kernel
    cl_program program = nullptr;
    const char *programName = "emu_benchmark_int64_div.cl";
    const char *kernelName = "int64_div";
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, nullptr, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(divisor), &divisor));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(loopIterations), &loopIterations));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, eventForEnqueue));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    if (eventForEnqueue) {
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Verify
    std::byte result[8] = {};
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, sizeof(cl_long), result, 0, nullptr, nullptr));
    if (std::memcmp(result, &initialValue, sizeof(cl_long)) != 0) {
        return TestResult::VerificationFail;
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<Int64Div> registerTestCase(run, Api::OpenCL);
