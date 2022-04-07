/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_memset.h"

#include <gtest/gtest.h>

static TestResult run(const UsmMemsetArguments &arguments, Statistics &statistics) {
    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).setProfiling(arguments.useEvents).allowCreationFail();
    Opencl opencl(queueProperties);
    if (opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!QueueFamiliesHelper::validateCapability(opencl.commandQueue, CL_QUEUE_CAPABILITY_FILL_BUFFER_INTEL)) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    auto clEnqueueMemsetINTEL = (pfn_clEnqueueMemsetINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemsetINTEL");
    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }

    // Create buffer
    UsmHelperOcl::Alloc dstAlloc{};
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, arguments.usmMemoryPlacement, arguments.bufferSize, dstAlloc));

    // Warmup
    const uint8_t memsetValue = 0x1;
    ASSERT_CL_SUCCESS(clEnqueueMemsetINTEL(opencl.commandQueue, dstAlloc.ptr, memsetValue, arguments.bufferSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillUsmBufferOrHostPtr(opencl.commandQueue, dstAlloc.ptr, arguments.bufferSize, dstAlloc.placement, arguments.contents));

        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueMemsetINTEL(opencl.commandQueue, dstAlloc.ptr, memsetValue, arguments.bufferSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), arguments.bufferSize, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), arguments.bufferSize, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
        }
    }

    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(dstAlloc));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmMemset> registerTestCase(run, Api::OpenCL, true);
