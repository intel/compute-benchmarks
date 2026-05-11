/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/event_query_profiling_data.h"

#include <gtest/gtest.h>
#include <vector>

static TestResult run(const EventQueryProfilingDataArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(true);
    Opencl opencl(queueProperties);
    cl_int retVal{};
    Timer timer;

    constexpr size_t bufferSize = 128u * MemoryConstants::kiloByte;
    size_t totalSize{};
    size_t offset = 0u;

    switch (arguments.splitCount) {
    case 1u:
        totalSize = bufferSize;
        break;
    case 2u:
        totalSize = bufferSize + 4u;
        break;
    case 3u:
        totalSize = bufferSize + 68u;
        offset = 4u;
        break;
    default:
        return TestResult::InvalidArgs;
    }

    const size_t fillSize = totalSize - offset;

    // Create buffer
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, totalSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const cl_uchar pattern = 0xAB;

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; ++i) {
        std::vector<cl_event> events(arguments.eventCount);
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            ASSERT_CL_SUCCESS(clEnqueueFillBuffer(opencl.commandQueue, buffer, &pattern, sizeof(pattern), offset, fillSize, 0, nullptr, &events[j]));
        }

        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        timer.measureStart();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            cl_ulong value{};
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(events[j], CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &value, nullptr));
        }
        timer.measureEnd();
        const auto startTime = timer.get();
        statistics.pushValue(startTime / arguments.eventCount, typeSelector.getUnit(), typeSelector.getType(), "Initial");

        timer.measureStart();
        for (auto j = 0u; j < arguments.eventCount; ++j) {
            cl_ulong value{};
            ASSERT_CL_SUCCESS(clGetEventProfilingInfo(events[j], CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &value, nullptr));
        }
        timer.measureEnd();
        const auto endTime = timer.get();
        statistics.pushValue(endTime / arguments.eventCount, typeSelector.getUnit(), typeSelector.getType(), "Cached");

        statistics.pushValue((startTime + endTime) / (2u * arguments.eventCount), typeSelector.getUnit(), typeSelector.getType(), "Average");

        for (auto j = 0u; j < arguments.eventCount; ++j) {
            ASSERT_CL_SUCCESS(clReleaseEvent(events[j]));
        }
    }

    // Verify
    std::vector<uint8_t> hostBuffer(totalSize);
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, totalSize, hostBuffer.data(), 0, nullptr, nullptr));

    for (size_t i = 0u; i < offset; i++) {
        if (hostBuffer[i] == pattern) {
            return TestResult::VerificationFail;
        }
    }

    std::vector<uint8_t> expected(fillSize, pattern);
    if (memcmp(hostBuffer.data() + offset, expected.data(), fillSize) != 0) {
        return TestResult::VerificationFail;
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<EventQueryProfilingData> registerTestCase(run, Api::OpenCL);
