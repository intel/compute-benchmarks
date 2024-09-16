/*
 * Copyright (C) 2022-2024 Intel Corporation
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

#include "definitions/usm_copy.h"

#include <gtest/gtest.h>

static TestResult run(const UsmCopyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (arguments.reuseCommandList == true ||
        requiresImport(arguments.sourcePlacement) ||
        requiresImport(arguments.sourcePlacement)) {
        return TestResult::ApiNotCapable;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents).setForceBlitter(arguments.forceBlitter).allowCreationFail();
    Opencl opencl(queueProperties);
    if (opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!QueueFamiliesHelper::validateCapabilities(opencl.commandQueue, CL_QUEUE_CAPABILITY_TRANSFER_BUFFER_INTEL, CL_QUEUE_CAPABILITY_FILL_BUFFER_INTEL)) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    auto clEnqueueMemcpyINTEL = (pfn_clEnqueueMemcpyINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemcpyINTEL");
    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }

    // Create buffers
    UsmHelperOcl::Alloc srcAlloc{};
    UsmHelperOcl::Alloc dstAlloc{};
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, arguments.sourcePlacement, arguments.size, srcAlloc));
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, arguments.destinationPlacement, arguments.size, dstAlloc));

    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillUsmBufferOrHostPtr(opencl.commandQueue, srcAlloc.ptr, arguments.size, srcAlloc.placement, arguments.contents));
    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillUsmBufferOrHostPtr(opencl.commandQueue, dstAlloc.ptr, arguments.size, dstAlloc.placement, arguments.contents));

    // Warmup
    ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(opencl.commandQueue, CL_FALSE, dstAlloc.ptr, srcAlloc.ptr, arguments.size, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(opencl.commandQueue, CL_FALSE, dstAlloc.ptr, srcAlloc.ptr, arguments.size, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), arguments.size, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(srcAlloc));
    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(dstAlloc));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopy> registerTestCase(run, Api::OpenCL, true);
