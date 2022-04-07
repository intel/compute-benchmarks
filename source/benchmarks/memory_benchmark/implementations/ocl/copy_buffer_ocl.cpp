/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/compression_helper.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_buffer.h"

#include <gtest/gtest.h>

static TestResult run(const CopyBufferArguments &arguments, Statistics &statistics) {
    if ((arguments.compressedDestination || arguments.compressedSource) && arguments.noIntelExtensions) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    cl_int retVal;
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);
    if (opencl.device == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    // Create buffers
    const cl_mem_flags compressionHintSrc = CompressionHelper::getCompressionFlags(arguments.compressedSource, arguments.noIntelExtensions);
    const cl_mem_flags compressionHintDst = CompressionHelper::getCompressionFlags(arguments.compressedDestination, arguments.noIntelExtensions);
    const cl_mem source = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | compressionHintSrc, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    const cl_mem destination = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | compressionHintDst, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Check buffers compression
    const auto srcCompressionStatus = CompressionHelper::verifyCompression(source, arguments.compressedSource, arguments.noIntelExtensions);
    const auto dstCompressionStatus = CompressionHelper::verifyCompression(destination, arguments.compressedDestination, arguments.noIntelExtensions);
    if (srcCompressionStatus != TestResult::Success || dstCompressionStatus != TestResult::Success) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(source));
        ASSERT_CL_SUCCESS(clReleaseMemObject(destination));
        if (srcCompressionStatus != TestResult::Success) {
            return srcCompressionStatus;
        } else {
            return dstCompressionStatus;
        }
    }

    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, source, arguments.size, arguments.contents));
    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, destination, arguments.size, arguments.contents));

    // Warmup
    ASSERT_CL_SUCCESS(clEnqueueCopyBuffer(opencl.commandQueue, source, destination, 0, 0, arguments.size, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueCopyBuffer(opencl.commandQueue, source, destination, 0, 0, arguments.size, 0, nullptr, eventForEnqueue));
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

    ASSERT_CL_SUCCESS(clReleaseMemObject(destination));
    ASSERT_CL_SUCCESS(clReleaseMemObject(source));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopyBuffer> registerTestCase(run, Api::OpenCL);
