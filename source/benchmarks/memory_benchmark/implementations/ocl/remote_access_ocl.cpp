/*
 * Copyright (C) 2022-2026 Intel Corporation
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

#include "definitions/remote_access.h"

#include <gtest/gtest.h>

using namespace MemoryConstants;

static TestResult run(const RemoteAccessMemoryArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    cl_int retVal = {};
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true).setOoq(0);
    Opencl opencl(queueProperties);

    auto subDeviceCount = 0u;
    const cl_device_partition_property properties[] = {CL_DEVICE_PARTITION_BY_AFFINITY_DOMAIN, CL_DEVICE_AFFINITY_DOMAIN_NUMA, 0};
    clCreateSubDevices(opencl.device, properties, 0u, nullptr, &subDeviceCount);
    if (subDeviceCount == 0u) {
        return TestResult::DeviceNotCapable;
    }

    Timer timer;

    size_t elementSize = sizeof(cl_double);
    const size_t fillValue = 313u;
    const uint32_t scalarValue = static_cast<uint32_t>(arguments.workItemPackSize);
    const size_t n_th = arguments.remoteFraction;

    // Create kernel-specific buffers
    const char *kernelName = {};
    const size_t bufferSize = arguments.size;
    cl_mem buffers[3] = {};
    size_t buffersCount = {};
    size_t bufferSizes[3] = {bufferSize, bufferSize, bufferSize};

    switch (arguments.type) {
    case StreamMemoryType::Read:
        kernelName = "remote_read";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        bufferSizes[buffersCount] = elementSize;
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, elementSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Write:
        kernelName = "remote_write";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Triad:
        kernelName = "remote_triad";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Scale:
        return TestResult::NoImplementation;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    // Create kernel
    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", "double");
    const char *programName = "memory_benchmark_stream_memory.cl";
    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.str().c_str(), program); result != TestResult::Success) {
        size_t numBytes = 0;
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes));
        auto buffer = std::make_unique<char[]>(numBytes);
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, buffer.get(), &numBytes));
        std::cout << buffer.get() << std::endl;

        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    for (auto i = 0u; i < buffersCount; i++) {
        ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffers[i], &fillValue, sizeof(fillValue), 0, bufferSizes[i], 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(i), sizeof(buffers[i]), &buffers[i]))
    }

    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount), sizeof(scalarValue), &scalarValue));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount + 1), sizeof(n_th), &n_th));

    // Query max workgroup size
    size_t maxWorkgroupSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr);

    const size_t globalWorkSize = arguments.size / elementSize;
    const size_t localWorkSize = maxWorkgroupSize;

    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        size_t transferSize = bufferSize;
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

static RegisterTestCaseImplementation<RemoteAccessMemory> registerTestCase(run, Api::OpenCL);
