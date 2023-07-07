/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/enqueue_barrier_with_empty_waitlist.h"

#include <cstring>
#include <gtest/gtest.h>

static TestResult run(const EnqueueBarrierWithEmptyWaitlistArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setOoq(arguments.outOfOrderQueue);
    Opencl opencl(queueProperties);
    Timer timer{};
    cl_int retVal{};

    // Prepare data
    const size_t workgroupCount = 128;
    const size_t lws = 128;
    const size_t gws = lws * workgroupCount;
    const size_t enqueueCount = arguments.enqueueCount;
    if (enqueueCount == 0) {
        return TestResult::InvalidArgs;
    }

    // Create kernel
    cl_program program = nullptr;
    const char *programName = "ulls_benchmark_eat_time.cl";
    const char *kernelName = "eat_time";
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, nullptr, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    const cl_int operationsCount = 1;
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(size_t), &operationsCount));

    // Warmup
    {
        cl_event event{};
        for (auto j = 0u; j < enqueueCount - 1; ++j) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clEnqueueBarrierWithWaitList(opencl.commandQueue, 0, nullptr, nullptr));
        }
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueBarrierWithWaitList(opencl.commandQueue, 0, nullptr, &event));

        ASSERT_CL_SUCCESS(clWaitForEvents(1, &event));
        ASSERT_CL_SUCCESS(clReleaseEvent(event));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        {
            cl_event event{};
            for (auto j = 0u; j < enqueueCount - 1; ++j) {
                ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
                ASSERT_CL_SUCCESS(clEnqueueBarrierWithWaitList(opencl.commandQueue, 0, nullptr, nullptr));
            }
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clEnqueueBarrierWithWaitList(opencl.commandQueue, 0, nullptr, &event));

            ASSERT_CL_SUCCESS(clWaitForEvents(1, &event));
            ASSERT_CL_SUCCESS(clReleaseEvent(event));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<EnqueueBarrierWithEmptyWaitlist> registerTestCase(run, Api::OpenCL);
