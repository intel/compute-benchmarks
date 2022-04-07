/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/write_buffer.h"

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/compression_helper.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include <gtest/gtest.h>

static TestResult run(const WriteBufferArguments &arguments, Statistics &statistics) {
    if (arguments.compressed && arguments.noIntelExtensions) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    cl_int retVal;
    QueueProperties queueProperties = QueueProperties::create().setDeviceSelection(arguments.queuePlacement).setProfiling(arguments.useEvents);
    ContextProperties contextProperties = ContextProperties::create().setDeviceSelection(arguments.contextPlacement).allowCreationFail();
    Opencl opencl(queueProperties, contextProperties);
    if (opencl.context == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    auto clCreateBufferWithPropertiesINTEL = (pfn_clCreateBufferWithPropertiesINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clCreateBufferWithPropertiesINTEL");
    if (!clCreateBufferWithPropertiesINTEL) {
        return TestResult::DriverFunctionNotFound;
    }
    Timer timer;

    // Create buffers
    const cl_mem_properties_intel memPropertiesDst[] = {
        CL_MEM_FLAGS,
        CL_MEM_READ_WRITE | CompressionHelper::getCompressionFlags(arguments.compressed, arguments.noIntelExtensions),
        CL_MEM_DEVICE_ID_INTEL,
        (cl_mem_properties_intel)opencl.getDevice(arguments.bufferPlacement),
        0,
    };
    const cl_mem buffer = clCreateBufferWithPropertiesINTEL(opencl.context, memPropertiesDst, 0, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Check buffer compression
    const auto compressionStatus = CompressionHelper::verifyCompression(buffer, arguments.compressed, arguments.noIntelExtensions);
    if (compressionStatus != TestResult::Success) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
        return compressionStatus;
    }

    // Fill buffer
    const char pattern[] = {0};
    ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffer, pattern, sizeof(pattern) / sizeof(pattern[0]), 0, arguments.size, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Warmup
    auto cpuBuffer = std::make_unique<uint8_t[]>(arguments.size);
    ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, arguments.size, cpuBuffer.get(), 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffer, CL_NON_BLOCKING, 0, arguments.size, cpuBuffer.get(), 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
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

    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<WriteBuffer> registerTestCase(run, Api::OpenCL, true);
