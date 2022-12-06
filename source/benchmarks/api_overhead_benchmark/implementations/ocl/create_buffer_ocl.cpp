/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/create_buffer.h"

#include <gtest/gtest.h>

static TestResult run(const CreateBufferArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Opencl opencl;
    Timer timer;
    cl_int retVal{};
    cl_mem_flags memFlags{};
    int *hostPtr = nullptr;
    std::vector<std::unique_ptr<int[]>> srcCpuBuffers(arguments.iterations);
    std::vector<cl_mem> buffersToRelease;
    if (arguments.copyHostPtr) {
        memFlags |= CL_MEM_COPY_HOST_PTR;
        for (auto i = 0u; i < arguments.iterations; ++i) {
            srcCpuBuffers[i] = std::make_unique<int[]>(arguments.bufferSize / sizeof(int));
        }
        hostPtr = srcCpuBuffers[0].get();
    }
    if (arguments.forceHostMemoryIntel) {
        memFlags |= CL_MEM_FORCE_HOST_MEMORY_INTEL;
    }
    if (arguments.readOnly) {
        memFlags |= CL_MEM_READ_ONLY;
    }
    // Warmup
    {
        cl_mem buffer = clCreateBuffer(opencl.context, memFlags, arguments.bufferSize, hostPtr, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.copyHostPtr) {
            hostPtr = srcCpuBuffers[i].get();
        }
        timer.measureStart();
        cl_mem buffer = clCreateBuffer(opencl.context, memFlags, arguments.bufferSize, hostPtr, &retVal);
        timer.measureEnd();
        ASSERT_CL_SUCCESS(retVal);
        if (arguments.allocateAll) {
            buffersToRelease.push_back(buffer);
        } else {
            ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (auto bufferToRelease : buffersToRelease) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferToRelease));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CreateBuffer> registerTestCase(run, Api::OpenCL);
