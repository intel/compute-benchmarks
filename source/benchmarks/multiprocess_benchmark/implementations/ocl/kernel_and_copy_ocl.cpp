/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_and_copy.h"

#include <gtest/gtest.h>

static TestResult run(const KernelAndCopyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (arguments.noIntelExtensions && arguments.useCopyQueue) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties);
    Timer timer{};
    cl_int retVal{};

    // Create queues
    const auto queueForCopyProperties = QueueProperties::create().setForceBlitter(arguments.useCopyQueue).allowCreationFail();
    const auto queueForKernelPropertes = QueueProperties::create();
    cl_command_queue queueForKernel{};
    cl_command_queue queueForCopy{};
    if (arguments.twoQueues) {
        queueForKernel = opencl.createQueue(queueForKernelPropertes);
        queueForCopy = opencl.createQueue(queueForCopyProperties);
    } else {
        if (arguments.runKernel) {
            queueForKernel = opencl.createQueue(queueForKernelPropertes);
            if (arguments.runCopy) {
                FATAL_ERROR_IF(arguments.useCopyQueue, "Configuration (runKernel && useCopyQueue && !twoQueues) is invalid");
                queueForCopy = queueForKernel;
            }
        } else if (arguments.runCopy) {
            queueForCopy = opencl.createQueue(queueForCopyProperties);
            ASSERT_CL_SUCCESS(retVal);
        } else {
            FATAL_ERROR("Either runCopy or runKernel must be active");
        }
    }

    // Validate copy queue creation (device may not support a BCS queue)
    if (arguments.runCopy && queueForCopy == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    // Create buffers
    const size_t bufferForKernelSize = 1024 * 600;
    const size_t bufferForCopySize = 1024 * 1024 * 512;
    cl_mem bufferForCopy1{};
    cl_mem bufferForCopy2{};
    cl_mem bufferForKernel{};
    if (arguments.runKernel) {
        bufferForKernel = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferForKernelSize, nullptr, &retVal);
    }
    if (arguments.runCopy) {
        bufferForCopy1 = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferForCopySize, nullptr, &retVal);
        bufferForCopy2 = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferForCopySize, nullptr, &retVal);
    }

    // Create kernel
    const char *source = "__kernel void fill_with_ones(__global char *buffer, int size) { "
                         "    for(int i=0; i < size; i++) {"
                         "        buffer[i] = 1;"
                         "    }"
                         "}";
    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel kernel = clCreateKernel(program, "fill_with_ones", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    // Warmup
    const size_t lws = 1;
    const size_t gws = 1;
    if (arguments.runKernel) {
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(bufferForKernel), &bufferForKernel));
        ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(bufferForKernelSize), &bufferForKernelSize));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queueForKernel, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queueForKernel));
    }
    if (arguments.runCopy) {
        ASSERT_CL_SUCCESS(clEnqueueCopyBuffer(queueForCopy, bufferForCopy1, bufferForCopy2, 0, 0, bufferForCopySize, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queueForCopy));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        // Enqueue
        if (arguments.runKernel) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queueForKernel, kernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        }
        if (arguments.runCopy) {
            ASSERT_CL_SUCCESS(clEnqueueCopyBuffer(queueForCopy, bufferForCopy1, bufferForCopy2, 0, 0, bufferForCopySize, 0, nullptr, nullptr));
        }

        // Flush
        if (arguments.runKernel) {
            ASSERT_CL_SUCCESS(clFlush(queueForKernel));
        }
        if (arguments.runCopy) {
            ASSERT_CL_SUCCESS(clFlush(queueForCopy));
        }

        // Measure finish
        timer.measureStart();
        if (arguments.runKernel) {
            ASSERT_CL_SUCCESS(clFinish(queueForKernel));
        }
        if (arguments.runCopy) {
            ASSERT_CL_SUCCESS(clFinish(queueForCopy));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    if (arguments.runKernel) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferForKernel));
    }
    if (arguments.runCopy) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferForCopy1));
        ASSERT_CL_SUCCESS(clReleaseMemObject(bufferForCopy2));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelAndCopy> registerTestCase(run, Api::OpenCL);
