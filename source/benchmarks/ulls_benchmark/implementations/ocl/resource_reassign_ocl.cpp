/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/intel_product/get_intel_product_ocl.h"
#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/queue_families_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/resource_reassign.h"

#include <array>
#include <gtest/gtest.h>
#include <thread>

static TestResult run(const ResourceReassignArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Latency, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    Opencl opencl(queueProperties);
    Timer timer{};
    cl_int retVal{};

    if (getIntelProduct(opencl) == IntelProduct::Unknown) {
        return TestResult::DeviceNotCapable;
    }

    if (QueueFamiliesHelper::getQueueCountForEngineGroup(opencl.device, EngineGroup::Compute) < 2) {
        return TestResult::DeviceNotCapable;
    }

    std::array<cl_command_queue, 4> queues{};
    for (size_t i = 0; i < arguments.queueCount + 1; i++) {
        queues[i] = opencl.createQueue(QueueProperties::create().setForceEngine(static_cast<Engine>(static_cast<uint32_t>(Engine::Ccs0) + i)));
    }

    size_t gws = 512 * 8 * 32;
    int count = 100;

    const char *source = "__kernel void stress(int count) { "
                         "    volatile int value = 1;"
                         "    for (int i = 0; i < count; i++) {"
                         "        for (int j = 0; j < count; j++) {"
                         "            value *= (i * j + count + 1);"
                         "            value /= 2;"
                         "        }"
                         "    }"
                         "}";

    const auto sourceLength = strlen(source);
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));
    cl_kernel stress = clCreateKernel(program, "stress", &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clSetKernelArg(stress, 0, sizeof(count), &count));

    // Warmup
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(queues[0]));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(queues[0]));
    for (size_t i = 1; i < arguments.queueCount + 1; i++) {
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[i], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queues[i]));
    }

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queues[0]));

        timer.measureStart();

        for (size_t i = 0; i < arguments.queueCount; i++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clFinish(queues[0]));
        }

        timer.measureEnd();
        auto singleQueueDiff = timer.get();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[0], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(queues[0]));

        timer.measureStart();

        for (size_t i = 1; i < arguments.queueCount + 1; i++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(queues[i], stress, 1, nullptr, &gws, nullptr, 0, nullptr, nullptr));
            ASSERT_CL_SUCCESS(clFinish(queues[i]));
        }

        timer.measureEnd();
        auto doubleQueueDiff = timer.get();

        auto value = doubleQueueDiff - singleQueueDiff;

        statistics.pushValue(value, typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(stress));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ResourceReassign> registerTestCase(run, Api::OpenCL);
