/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_migrate_gpu_for_fill.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedMigrateGpuForFillArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    Opencl opencl(queueProperties);
    Timer timer;
    if (!QueueFamiliesHelper::validateCapability(opencl.commandQueue, CL_QUEUE_CAPABILITY_FILL_BUFFER_INTEL)) {
        return TestResult::DeviceNotCapable;
    }
    auto clSharedMemAllocINTEL = (pfn_clSharedMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clSharedMemAllocINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clMemFreeINTEL");
    auto clEnqueueMigrateMemINTEL = (pfn_clEnqueueMigrateMemINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMigrateMemINTEL");
    auto clEnqueueMemFillINTEL = (pfn_clEnqueueMemFillINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemFillINTEL");
    if (!clSharedMemAllocINTEL || !clMemFreeINTEL || !clEnqueueMigrateMemINTEL || !clEnqueueMemFillINTEL) {
        return TestResult::DriverFunctionNotFound;
    }
    cl_int retVal{};

    // Create buffer
    auto buffer = static_cast<cl_int *>(clSharedMemAllocINTEL(opencl.context, opencl.device, nullptr, arguments.bufferSize, 0u, &retVal));
    ASSERT_CL_SUCCESS(retVal);
    const size_t elementsCount = arguments.bufferSize / sizeof(cl_int);
    const uint8_t pattern = 1;

    // Warmup
    if (arguments.prefetchMemory) {
        ASSERT_CL_SUCCESS(clEnqueueMigrateMemINTEL(opencl.commandQueue, buffer, arguments.bufferSize, 0, 0, nullptr, nullptr));
    }

    ASSERT_CL_SUCCESS(clEnqueueMemFillINTEL(opencl.commandQueue, buffer, &pattern, 1, arguments.bufferSize, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        // Migrate whole resource to CPU
        for (auto elementIndex = 0u; elementIndex < elementsCount; elementIndex++) {
            buffer[elementIndex] = 0;
        }

        // Measure memory fill operation which must migrate the resource to GPU
        timer.measureStart();

        if (arguments.prefetchMemory) {
            ASSERT_CL_SUCCESS(clEnqueueMigrateMemINTEL(opencl.commandQueue, buffer, arguments.bufferSize, 0, 0, nullptr, nullptr));
        }

        ASSERT_CL_SUCCESS(clEnqueueMemFillINTEL(opencl.commandQueue, buffer, &pattern, 1, arguments.bufferSize, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.bufferSize, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedMigrateGpuForFill> registerTestCase(run, Api::OpenCL, true);
