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

#include "definitions/tmp_buffer_mixed_size.h"

#include <gtest/gtest.h>

namespace {

struct QueueData {
    ur_queue_handle_t queue;
    void *tmpBuffer;
};

enum class SizeVariant {
    Small,
    Big,
};

TestResult execKernel(UrState &ur, ur_kernel_handle_t kernel, QueueData queueData, const TmpBufferMixedSizeArguments &arguments, SizeVariant var) {
    size_t bufferSize = (var == SizeVariant::Small ? arguments.sizeSmall : arguments.sizeSmall * arguments.sizeLargeRatio);

    void *tmpBuffer = queueData.tmpBuffer;
    if (arguments.strategy == TmpMemoryStrategy::Sync) {
        ASSERT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, bufferSize, &tmpBuffer));
    } else if (arguments.strategy == TmpMemoryStrategy::Async) {
        ASSERT_UR_RESULT_SUCCESS(urEnqueueUSMDeviceAllocExp(queueData.queue, nullptr, bufferSize, nullptr, 0, nullptr, &tmpBuffer, nullptr));
    }

    ASSERT_UR_RESULT_SUCCESS(urKernelSetArgValue(kernel, 0, sizeof(tmpBuffer), nullptr, reinterpret_cast<void *>(&tmpBuffer)));

    constexpr size_t globalOffset[] = {0};
    size_t globalSize[] = {bufferSize / sizeof(int)};
    ASSERT_UR_RESULT_SUCCESS(urEnqueueKernelLaunch(queueData.queue, kernel, 1, globalOffset, globalSize, nullptr, 0, nullptr, 0, nullptr, nullptr));

    if (arguments.strategy == TmpMemoryStrategy::Sync) {
        ASSERT_UR_RESULT_SUCCESS(urQueueFinish(queueData.queue));
        ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, tmpBuffer));
    } else if (arguments.strategy == TmpMemoryStrategy::Async) {
        ASSERT_UR_RESULT_SUCCESS(urEnqueueUSMFreeExp(queueData.queue, nullptr, tmpBuffer, 0, nullptr, nullptr));
    }

    return TestResult::Success;
}

// In this scenario, we use two queues to execute kernels on small and large buffers.
// Async allocation API enables buffers to be reused between the queues.
TestResult scenario(UrState &ur, ur_kernel_handle_t kernel, QueueData queueDataA, QueueData queueDataB, const TmpBufferMixedSizeArguments &arguments) {
    size_t sizeLarge = arguments.sizeSmall * arguments.sizeLargeRatio;
    if (arguments.strategy == TmpMemoryStrategy::Static) {
        ASSERT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, sizeLarge, &queueDataA.tmpBuffer));
        ASSERT_UR_RESULT_SUCCESS(urUSMDeviceAlloc(ur.context, ur.device, nullptr, nullptr, sizeLarge, &queueDataB.tmpBuffer));
    }

    ASSERT_TEST_RESULT_SUCCESS(execKernel(ur, kernel, queueDataA, arguments, SizeVariant::Big));
    for (auto i = 0u; i < arguments.numKernelsSmall; i++)
        ASSERT_TEST_RESULT_SUCCESS(execKernel(ur, kernel, queueDataA, arguments, SizeVariant::Small));

    for (auto i = 0u; i < arguments.numKernelsSmall; i++)
        ASSERT_TEST_RESULT_SUCCESS(execKernel(ur, kernel, queueDataB, arguments, SizeVariant::Small));
    ASSERT_TEST_RESULT_SUCCESS(execKernel(ur, kernel, queueDataB, arguments, SizeVariant::Big));

    ASSERT_UR_RESULT_SUCCESS(urQueueFinish(queueDataA.queue));
    ASSERT_UR_RESULT_SUCCESS(urQueueFinish(queueDataB.queue));

    if (arguments.strategy == TmpMemoryStrategy::Static) {
        ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, queueDataA.tmpBuffer));
        ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, queueDataB.tmpBuffer));
    }
    return TestResult::Success;
}

} // namespace

static TestResult run(const TmpBufferMixedSizeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
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

    ur_queue_properties_t queueProperties = {};
    QueueData queueDataA, queueDataB;
    ASSERT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device, &queueProperties, &queueDataA.queue));
    ASSERT_UR_RESULT_SUCCESS(urQueueCreate(ur.context, ur.device, &queueProperties, &queueDataB.queue));

    // Buffer size in bytes must be multiple of sizeof(int)
    if (arguments.sizeSmall % sizeof(int) != 0)
        return TestResult::InvalidArgs;

    // Warmup
    ASSERT_TEST_RESULT_SUCCESS(scenario(ur, kernel, queueDataA, queueDataB, arguments));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(scenario(ur, kernel, queueDataA, queueDataB, arguments));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queueDataA.queue));
    EXPECT_UR_RESULT_SUCCESS(urQueueRelease(queueDataB.queue));
    EXPECT_UR_RESULT_SUCCESS(urKernelRelease(kernel));
    EXPECT_UR_RESULT_SUCCESS(urProgramRelease(program));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<TmpBufferMixedSize> registerTestCase(run, Api::UR);
