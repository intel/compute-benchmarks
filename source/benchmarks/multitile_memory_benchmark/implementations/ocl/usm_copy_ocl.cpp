/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/compression_helper.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyArguments &arguments, Statistics &statistics) {
    // Setup
    cl_int retVal;
    QueueProperties queueProperties = QueueProperties::create().setDeviceSelection(arguments.queuePlacement).setForceBlitter(arguments.forceBlitter).setProfiling(arguments.useEvents).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create().setDeviceSelection(arguments.contextPlacement).allowCreationFail();
    Opencl opencl(queueProperties, contextProperties);
    if (opencl.context == nullptr || opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!QueueFamiliesHelper::validateCapabilities(opencl.commandQueue, CL_QUEUE_CAPABILITY_FILL_BUFFER_INTEL)) {
        return TestResult::DeviceNotCapable;
    }
    auto clCreateBufferWithPropertiesINTEL = (pfn_clCreateBufferWithPropertiesINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clCreateBufferWithPropertiesINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clMemFreeINTEL");
    auto clEnqueueMemcpyINTEL = (pfn_clEnqueueMemcpyINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemcpyINTEL");
    if (!clCreateBufferWithPropertiesINTEL || !opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }
    Timer timer;

    // Create buffer
    void *src = UsmHelperOcl::allocate(arguments.srcPlacement, opencl, arguments.size, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    void *dst = UsmHelperOcl::allocate(arguments.dstPlacement, opencl, arguments.size, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(opencl.commandQueue, CL_FALSE, dst, src, arguments.size, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue))

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(opencl.commandQueue, CL_FALSE, dst, src, arguments.size, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue))
        timer.measureEnd();

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
        }
    }

    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, src));
    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, dst));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopy> registerTestCase(run, Api::OpenCL, true);
