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

#include "definitions/remote_access_max_saturation.h"

#include <gtest/gtest.h>

using namespace MemoryConstants;

static TestResult run(const RemoteAccessMaxSaturationArguments &arguments, Statistics &statistics) {
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

    const size_t bufferSize = arguments.size;
    const size_t n_th = arguments.remoteFraction;
    const uint32_t writesPerWorkgroup = arguments.writesPerWorkgroup;

    // Create kernel-specific buffers
    const char *kernelName = "remote_max_saturation";
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);

    // Create kernel
    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STREAM_TYPE", "double");
    const char *programName = "memory_benchmark_stream_memory.cl";
    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.str().c_str(), program); result != TestResult::Success) {
        size_t numBytes = 0;
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, 0, NULL, &numBytes));
        auto logBuffer = std::make_unique<char[]>(numBytes);
        ASSERT_CL_SUCCESS(clGetProgramBuildInfo(program, opencl.device, CL_PROGRAM_BUILD_LOG, numBytes, logBuffer.get(), &numBytes));
        std::cout << logBuffer.get() << std::endl;

        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffer, &fillValue, sizeof(fillValue), 0, bufferSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(0), sizeof(buffer), &buffer))
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(1), sizeof(cl_uint), &scalarValue));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(2), elementSize, &n_th));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, static_cast<cl_uint>(3), sizeof(cl_uint), &writesPerWorkgroup));

    // Query max workgroup size
    size_t maxWorkgroupSize = {};
    clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr);

    // Warm up
    const size_t globalWorkSize = arguments.size / elementSize;
    const size_t localWorkSize = maxWorkgroupSize;

    if (writesPerWorkgroup > localWorkSize) {
        return TestResult::DeviceNotCapable;
    }

    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalWorkSize, &localWorkSize, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        size_t transferSize = bufferSize / localWorkSize * writesPerWorkgroup;

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
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<RemoteAccessMaxSaturation> registerTestCase(run, Api::OpenCL);
