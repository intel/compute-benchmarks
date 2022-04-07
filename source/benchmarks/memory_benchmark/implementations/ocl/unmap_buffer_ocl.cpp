/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/compression_helper.h"
#include "framework/ocl/utility/map_flags_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/unmap_buffer.h"

#include <gtest/gtest.h>

static TestResult run(const UnmapBufferArguments &arguments, Statistics &statistics) {
    if (arguments.compressed && arguments.noIntelExtensions) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    Opencl opencl;
    Timer timer;
    cl_int retVal;

    // Create buffer
    const cl_mem_flags compressionHint = CompressionHelper::getCompressionFlags(arguments.compressed, arguments.noIntelExtensions);
    const cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | compressionHint, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Check buffer compression
    const auto compressionStatus = CompressionHelper::verifyCompression(buffer, arguments.compressed, arguments.noIntelExtensions);
    if (compressionStatus != TestResult::Success) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
        return compressionStatus;
    }

    // Fill buffer
    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, buffer, arguments.size, arguments.contents));

    // Warmup
    const auto mapFlags = convertMapFlags(arguments.mapFlags);
    void *ptr = clEnqueueMapBuffer(opencl.commandQueue, buffer, CL_BLOCKING, mapFlags, 0, arguments.size, 0, nullptr, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueUnmapMemObject(opencl.commandQueue, buffer, ptr, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ptr = clEnqueueMapBuffer(opencl.commandQueue, buffer, CL_BLOCKING, mapFlags, 0, arguments.size, 0, nullptr, nullptr, &retVal);
        ASSERT_CL_SUCCESS(retVal);

        BufferContentsHelperOcl::fill(static_cast<uint8_t *>(ptr), arguments.size, arguments.contents);

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueUnmapMemObject(opencl.commandQueue, buffer, ptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.size, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UnmapBuffer> registerTestCase(run, Api::OpenCL);
