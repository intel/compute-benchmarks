/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/stream_memory.h"

#include <gtest/gtest.h>

using namespace MemoryConstants;

static TestResult run(const StreamMemoryArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    cl_int retVal = {};
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true).setOoq(0);
    Opencl opencl(queueProperties);
    Timer timer;
    bool useDoubles = opencl.getExtensions().areDoublesSupported();

    size_t elementSize = useDoubles ? 8u : 4u;
    const int64_t scalarValue = -999;
    bool setScalarArgument = true;
    const bool printBuildInfo = true;

    // Create kernel-specific buffers
    const char *kernelName = {};
    size_t bufferSize = arguments.size;
    cl_mem buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {bufferSize, bufferSize, bufferSize};

    auto bufferFlags = CL_MEM_READ_WRITE;
    if (arguments.memoryPlacement == UsmMemoryPlacement::Host) {
        bufferFlags |= CL_MEM_FORCE_HOST_MEMORY_INTEL;
    }

    switch (arguments.type) {
    case StreamMemoryType::Read:
        kernelName = "read";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        bufferSizes[buffersCount] = 16u;
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, 16u, nullptr, &retVal);
        break;
    case StreamMemoryType::Write:
        if (BufferContents::Random == arguments.contents) {
            kernelName = "write_random";
            setScalarArgument = false; // value to write is embedded in kernel code
        } else {
            kernelName = "write";
        }
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Scale:
        kernelName = "scale";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Triad:
        kernelName = "triad";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, bufferFlags, bufferSize, nullptr, &retVal);
        break;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    // Create kernel
    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", useDoubles ? "double" : "float");
    const char *programName = "memory_benchmark_stream_memory.cl";
    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.str().c_str(), program); result != TestResult::Success) {
        if (result != TestResult::Success && printBuildInfo) {
            size_t numBytes = 0;
            retVal |= clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes);
            auto buffer = std::make_unique<char[]>(numBytes);
            retVal |= clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, buffer.get(), &numBytes);
            std::cout << buffer.get() << std::endl;
        }
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    for (auto i = 0u; i < buffersCount; i++) {
        ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, buffers[i], bufferSizes[i], arguments.contents));
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(i), sizeof(buffers[i]), &buffers[i]))
    }
    if (setScalarArgument) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount), elementSize, &scalarValue));
    }

    // Query max workgroup size
    size_t maxWorkgroupSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr);

    // Warm up
    const size_t globalWorkSize = arguments.size / elementSize;
    const size_t localWorkSize = std::min(maxWorkgroupSize, globalWorkSize);
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        size_t transferSize = arguments.size;
        switch (arguments.type) {
        case StreamMemoryType::Scale:
            transferSize *= 2;
            break;
        case StreamMemoryType::Triad:
            transferSize *= 3;
            break;
        default:
            break;
        }

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));

            statistics.pushValue(std::chrono::nanoseconds(timeNs), transferSize, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), transferSize, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Cleanup
    for (size_t i = 0; i < buffersCount; i++) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffers[i]));
    }
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<StreamMemory> registerTestCase(run, Api::OpenCL);
