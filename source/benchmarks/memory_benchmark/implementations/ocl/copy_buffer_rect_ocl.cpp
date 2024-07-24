/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/compression_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_buffer_rect.h"

#include <gtest/gtest.h>

static TestResult run(const CopyBufferRectArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if ((arguments.compressedDestination || arguments.compressedSource) && arguments.noIntelExtensions) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    Opencl opencl;
    Timer timer;
    cl_int retVal;

    // Create buffers
    const cl_mem_flags compressionHintSrc = CompressionHelper::getCompressionFlags(arguments.compressedSource, arguments.noIntelExtensions);
    const cl_mem_flags compressionHintDst = CompressionHelper::getCompressionFlags(arguments.compressedDestination, arguments.noIntelExtensions);
    const cl_mem sourceBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | compressionHintSrc, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    const cl_mem destinationBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE | compressionHintDst, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Check buffers compression
    const auto srcCompressionStatus = CompressionHelper::verifyCompression(sourceBuffer, arguments.compressedSource, arguments.noIntelExtensions);
    const auto dstCompressionStatus = CompressionHelper::verifyCompression(destinationBuffer, arguments.compressedDestination, arguments.noIntelExtensions);
    if (srcCompressionStatus != TestResult::Success || dstCompressionStatus != TestResult::Success) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(sourceBuffer));
        ASSERT_CL_SUCCESS(clReleaseMemObject(destinationBuffer));
        if (srcCompressionStatus != TestResult::Success) {
            return srcCompressionStatus;
        } else {
            return dstCompressionStatus;
        }
    }

    const auto copySize = (arguments.region[0] > 0 ? arguments.region[0] : 1u) * (arguments.region[1] > 0 ? arguments.region[1] : 1u) * (arguments.region[2] > 0 ? arguments.region[2] : 1u);

    // Warmup
    ASSERT_CL_SUCCESS(clEnqueueCopyBufferRect(opencl.commandQueue, sourceBuffer, destinationBuffer,
                                              arguments.origin, arguments.origin, arguments.region,
                                              arguments.rPitch, arguments.sPitch, arguments.rPitch, arguments.sPitch,
                                              0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueCopyBufferRect(opencl.commandQueue, sourceBuffer, destinationBuffer,
                                                  arguments.origin, arguments.origin, arguments.region,
                                                  arguments.rPitch, arguments.sPitch, arguments.rPitch, arguments.sPitch,
                                                  0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue))
        timer.measureEnd();
        statistics.pushValue(timer.get(), copySize, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(sourceBuffer));
    ASSERT_CL_SUCCESS(clReleaseMemObject(destinationBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopyBufferRect> registerTestCase(run, Api::OpenCL);
