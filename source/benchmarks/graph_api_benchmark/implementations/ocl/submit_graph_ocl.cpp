/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// Required to expose cl_khr_command_buffer declarations in cl_ext.h
#define CL_ENABLE_BETA_EXTENSIONS

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/submit_graph.h"

#include <gtest/gtest.h>

// Mirror cl_khr_command_buffer 0.9.7 declarations from cl_ext.h if compiling
// against an older version of OpenCL-Headers without cl_khr_command_buffer
// declarations
#ifndef cl_khr_command_buffer
#define CL_KHR_COMMAND_BUFFER_EXTENSION_NAME "cl_khr_command_buffer"

typedef struct _cl_command_buffer_khr *cl_command_buffer_khr;
typedef cl_ulong cl_command_buffer_properties_khr;
typedef cl_uint cl_sync_point_khr;
typedef struct _cl_mutable_command_khr *cl_mutable_command_khr;
typedef cl_ulong cl_command_properties_khr;

typedef cl_command_buffer_khr CL_API_CALL
clCreateCommandBufferKHR_t(
    cl_uint num_queues,
    const cl_command_queue *queues,
    const cl_command_buffer_properties_khr *properties,
    cl_int *errcode_ret);

typedef clCreateCommandBufferKHR_t *
    clCreateCommandBufferKHR_fn;

typedef cl_int CL_API_CALL
clFinalizeCommandBufferKHR_t(
    cl_command_buffer_khr command_buffer);

typedef clFinalizeCommandBufferKHR_t *
    clFinalizeCommandBufferKHR_fn;

typedef cl_int CL_API_CALL
clReleaseCommandBufferKHR_t(
    cl_command_buffer_khr command_buffer);

typedef clReleaseCommandBufferKHR_t *
    clReleaseCommandBufferKHR_fn;

typedef cl_int CL_API_CALL
clEnqueueCommandBufferKHR_t(
    cl_uint num_queues,
    cl_command_queue *queues,
    cl_command_buffer_khr command_buffer,
    cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list,
    cl_event *event);

typedef clEnqueueCommandBufferKHR_t *
    clEnqueueCommandBufferKHR_fn;

typedef cl_int CL_API_CALL
clCommandNDRangeKernelKHR_t(
    cl_command_buffer_khr command_buffer,
    cl_command_queue command_queue,
    const cl_command_properties_khr *properties,
    cl_kernel kernel,
    cl_uint work_dim,
    const size_t *global_work_offset,
    const size_t *global_work_size,
    const size_t *local_work_size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point,
    cl_mutable_command_khr *mutable_handle);

typedef clCommandNDRangeKernelKHR_t *
    clCommandNDRangeKernelKHR_fn;
#endif // #ifndef cl_khr_command_buffer

static TestResult run(const SubmitGraphArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    if (!arguments.emulateGraphs || arguments.useHostTasks || arguments.useExplicit) {
        return TestResult::ApiNotCapable;
    } else if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    auto queueProperties = QueueProperties::create().setProfiling(arguments.useProfiling).setOoq(!arguments.inOrderQueue);
    Opencl opencl(queueProperties);
    cl_int retVal{};

    // cl_khr_command_buffer extension is required
    if (!opencl.getExtensions().isSupported(CL_KHR_COMMAND_BUFFER_EXTENSION_NAME)) {
        return TestResult::DeviceNotCapable;
    }

    auto clCreateCommandBufferKHR = (clCreateCommandBufferKHR_fn)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clCreateCommandBufferKHR");
    auto clCommandNDRangeKernelKHR = (clCommandNDRangeKernelKHR_fn)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clCommandNDRangeKernelKHR");
    auto clFinalizeCommandBufferKHR = (clFinalizeCommandBufferKHR_fn)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clFinalizeCommandBufferKHR");
    auto clReleaseCommandBufferKHR = (clReleaseCommandBufferKHR_fn)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clReleaseCommandBufferKHR");
    auto clEnqueueCommandBufferKHR = (clEnqueueCommandBufferKHR_fn)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueCommandBufferKHR");

    Timer timer;
    const size_t gws = 1u;
    const size_t lws = 1u;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    cl_program program = clCreateProgramWithIL(opencl.context, spirvModule.data(), spirvModule.size(), &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "eat_time", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_int kernelOperationsCount = static_cast<cl_int>(arguments.kernelExecutionTime);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_int), &kernelOperationsCount));

    // Create command-buffer
    cl_command_buffer_khr cmdBuf = clCreateCommandBufferKHR(1, &opencl.commandQueue, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    for (auto i = 0u; i < arguments.numKernels; i++) {
        ASSERT_CL_SUCCESS(clCommandNDRangeKernelKHR(cmdBuf, nullptr, nullptr, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr, nullptr));
    }

    ASSERT_CL_SUCCESS(clFinalizeCommandBufferKHR(cmdBuf));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        if (!arguments.useEvents) {
            ASSERT_CL_SUCCESS(clEnqueueCommandBufferKHR(1, &opencl.commandQueue, cmdBuf, 0, nullptr, nullptr));
            if (!arguments.measureCompletionTime) {
                timer.measureEnd();
            }
            ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        } else {
            cl_event event;
            ASSERT_CL_SUCCESS(clEnqueueCommandBufferKHR(1, &opencl.commandQueue, cmdBuf, 0, nullptr, &event));
            if (!arguments.measureCompletionTime) {
                timer.measureEnd();
            }
            ASSERT_CL_SUCCESS(clWaitForEvents(1, &event));
            ASSERT_CL_SUCCESS(clReleaseEvent(event));
        }

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    EXPECT_CL_SUCCESS(clReleaseCommandBufferKHR(cmdBuf));
    EXPECT_CL_SUCCESS(clReleaseKernel(kernel));
    EXPECT_CL_SUCCESS(clReleaseProgram(program));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitGraph> registerTestCase(run, Api::OpenCL);
