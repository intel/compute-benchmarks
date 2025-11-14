/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/tmp_buffer_fixed_size.h"

#include <gtest/gtest.h>
#include <level_zero/zes_api.h>

namespace {

TestResult execKernel(UrState &ur, ur_kernel_handle_t kernel, ur_queue_handle_t queue, void *tmpBuffer, const TmpBufferFixedSizeArguments &arguments) {
    if (arguments.strategy == TmpMemoryStrategy::Sync) {
        ASSERT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, arguments.size, &tmpBuffer));
    } else if (arguments.strategy == TmpMemoryStrategy::Async) {
        ASSERT_UR_RESULT_SUCCESS(urEnqueueUSMDeviceAllocExp(queue, nullptr, arguments.size, nullptr, 0, nullptr, &tmpBuffer, nullptr));
    }

    ASSERT_UR_RESULT_SUCCESS(urKernelSetArgValue(kernel, 0, sizeof(tmpBuffer), nullptr, reinterpret_cast<void *>(&tmpBuffer)));

    constexpr size_t globalOffset[] = {0};
    size_t globalSize[] = {arguments.size / sizeof(int)};
    ASSERT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(queue, kernel, 1, globalOffset, globalSize, nullptr, nullptr, 0, nullptr, nullptr));

    if (arguments.strategy == TmpMemoryStrategy::Sync) {
        ASSERT_UR_RESULT_SUCCESS(urQueueFinish(queue));
        ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, tmpBuffer));
    } else if (arguments.strategy == TmpMemoryStrategy::Async) {
        ASSERT_UR_RESULT_SUCCESS(urEnqueueUSMFreeExp(queue, nullptr, tmpBuffer, 0, nullptr, nullptr));
    }

    return TestResult::Success;
}

// This scenario reflects usage of temporary buffers within procedures
// with no explicit synchronization in between.
TestResult scenario(UrState &ur, ur_kernel_handle_t kernel, ur_queue_handle_t queue, const TmpBufferFixedSizeArguments &arguments) {
    void *tmpBuffer;
    if (arguments.strategy == TmpMemoryStrategy::Static) {
        ASSERT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, arguments.size, &tmpBuffer));
    }

    for (auto i = 0u; i < arguments.numKernels; i++) {
        ASSERT_TEST_RESULT_SUCCESS(execKernel(ur, kernel, queue, tmpBuffer, arguments));
    }
    ASSERT_UR_RESULT_SUCCESS(urQueueFinish(queue));

    if (arguments.strategy == TmpMemoryStrategy::Static) {
        ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, tmpBuffer));
    }
    return TestResult::Success;
}

} // namespace

static TestResult run(const TmpBufferFixedSizeArguments &arguments, Statistics &statistics) {
    MeasurementFields timerTypeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(timerTypeSelector.getUnit(), timerTypeSelector.getType());
        return TestResult::Nooped;
    }

    UrState ur;
    Timer timer;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_fill_with_ones.spv");
    if (spirvModule.size() == 0)
        return TestResult::KernelNotFound;

    ur_program_handle_t program;
    EXPECT_UR_RESULT_SUCCESS(urProgramCreateWithIL(ur.context, spirvModule.data(),
                                                   spirvModule.size(), nullptr, &program));
    EXPECT_UR_RESULT_SUCCESS(urProgramBuild(ur.context, program, nullptr));

    ur_kernel_handle_t kernel;
    EXPECT_UR_RESULT_SUCCESS(urKernelCreate(program, "fill_with_ones", &kernel));

    ur_queue_handle_t queue;
    ur_queue_properties_t queueProperties = {};
    ASSERT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device, &queueProperties, &queue));

    // Buffer size in bytes must be multiple of sizeof(int)
    if (arguments.size % sizeof(int) != 0)
        return TestResult::InvalidArgs;

    // Warmup
    ASSERT_TEST_RESULT_SUCCESS(scenario(ur, kernel, queue, arguments));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(scenario(ur, kernel, queue, arguments));
        timer.measureEnd();
        statistics.pushValue(timer.get(), timerTypeSelector.getUnit(), timerTypeSelector.getType());
    }

    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queue));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernel));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<TmpBufferFixedSize> registerTestCase(run, Api::UR);
