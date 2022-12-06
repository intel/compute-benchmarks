/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_submission_events.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

static TestResult run(const CopySubmissionEventsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true).setForceEngine(arguments.engine).allowCreationFail();
    Opencl opencl(queueProperties);
    if (nullptr == opencl.commandQueue) {
        return TestResult::DeviceNotCapable;
    }
    cl_int retVal{};
    cl_event profilingEvent{};

    auto clHostMemAllocINTEL = (pfn_clHostMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clHostMemAllocINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clMemFreeINTEL");
    if (!clHostMemAllocINTEL || !clMemFreeINTEL) {
        return TestResult::DriverFunctionNotFound;
    }

    size_t transferSize = 2097152u;

    void *hostMemory = clHostMemAllocINTEL(opencl.context, nullptr, transferSize, 0, &retVal);
    const cl_mem destination = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, transferSize, nullptr, &retVal);

    // Warmup run
    ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, destination, CL_NON_BLOCKING, 0, transferSize, hostMemory, 0, nullptr, &profilingEvent));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue))
    clReleaseEvent(profilingEvent);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_ulong queued{}, start{};
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, destination, CL_NON_BLOCKING, 0, transferSize, hostMemory, 0, nullptr, &profilingEvent));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue))

        ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvent, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, nullptr));
        ASSERT_CL_SUCCESS(clGetEventProfilingInfo(profilingEvent, CL_PROFILING_COMMAND_QUEUED, sizeof(cl_ulong), &queued, nullptr));
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
        const auto submissionTime = std::chrono::nanoseconds(start - queued);
        statistics.pushValue(submissionTime, typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, hostMemory));
    ASSERT_CL_SUCCESS(clReleaseMemObject(destination));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopySubmissionEvents> registerTestCase(run, Api::OpenCL);
