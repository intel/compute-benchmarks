/*
 * Copyright (C) 2022-2023 Intel Corporation
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

#include "definitions/full_remote_access.h"

#include <gtest/gtest.h>

using namespace MemoryConstants;

static TestResult run(const FullRemoteAccessMemoryArguments &arguments, Statistics &statistics) {
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
    const uint64_t fillValue = 0xffffffffffffffff;

    const size_t bufferSize = arguments.size;
    const size_t elementSize = arguments.elementSize;
    const size_t workItems = arguments.workItems;
    const bool blockAccess = arguments.blockAccess;

    const auto bufferLength = bufferSize / elementSize;
    const auto iterations = bufferLength / workItems;

    // Create kernel-specific buffers
    const char *kernelName = {};
    cl_mem buffers[2] = {};
    size_t buffersCount = {};
    size_t bufferSizes[2] = {bufferSize, bufferSize};

    switch (arguments.type) {
    case StreamMemoryType::Read:
        kernelName = blockAccess ? "full_remote_block_read" : "full_remote_scatter_read";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        bufferSizes[buffersCount] = elementSize;
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, elementSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Write:
        kernelName = blockAccess ? "full_remote_block_write" : "full_remote_scatter_write";
        buffers[buffersCount++] = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
        break;
    case StreamMemoryType::Triad:
    case StreamMemoryType::Scale:
        return TestResult::NoImplementation;
    default:
        FATAL_ERROR("Unknown StreamMemoryType");
    }

    // Create kernel
    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("ELEMENT_SIZE", elementSize);
    if (elementSize == 1)
        compilerOptions.addDefinitionKeyValue("STREAM_TYPE", "char");
    else if (elementSize == 2)
        compilerOptions.addDefinitionKeyValue("STREAM_TYPE", "short");
    else if (elementSize == 4)
        compilerOptions.addDefinitionKeyValue("STREAM_TYPE", "int");
    else if (elementSize == 8)
        compilerOptions.addDefinitionKeyValue("STREAM_TYPE", "long");
    else
        return TestResult::InvalidArgs;

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
        ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffers[i], &fillValue, elementSize, 0, bufferSizes[i], 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(i), sizeof(buffers[i]), &buffers[i]))
    }
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount), sizeof(cl_uint), &bufferLength));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(buffersCount + 1), sizeof(cl_uint), &iterations));

    // Query max workgroup size
    size_t maxWorkgroupSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr);

    // Warm up
    const size_t globalWorkSize = workItems;
    const size_t localWorkSize = workItems > maxWorkgroupSize ? maxWorkgroupSize : workItems;

    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        size_t transferSize = bufferSize;

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

static RegisterTestCaseImplementation<FullRemoteAccessMemory> registerTestCase(run, Api::OpenCL);
