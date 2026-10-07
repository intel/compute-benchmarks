/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/submit_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const SubmitKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    auto queueProperties = QueueProperties::create().setProfiling(arguments.useProfiling).setOoq(!arguments.inOrderQueue);
    Opencl opencl(queueProperties);
    cl_int retVal{};

    Timer timer;
    const size_t gws = 1u;
    const size_t lws = 1u;

    cl_program program = nullptr;
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "api_overhead_benchmark_eat_time.cl", nullptr, program);
        result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "eat_time", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);

    cl_event event{};
    cl_event *eventPtr = arguments.useEvents ? &event : nullptr;

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
            // Note: this test calls clSetKernelArg each time to be closer to the SYCL behavior!
            ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(int), &kernelOperationsCount));
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, eventPtr));
            // Note: this test calls clReleaseEvent immediately after enqueuing the kernel to be closer to the SYCL behavior!
            if (arguments.useEvents) {
                ASSERT_CL_SUCCESS(clReleaseEvent(event));
            }
        }

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernel> registerTestCase(run, Api::OpenCL);
