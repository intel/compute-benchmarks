/*
 * Copyright (C) 2025-2026 Intel Corporation
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

    ur_exp_kernel_arg_value_t val = {};
    val.pointer = tmpBuffer;

    ur_exp_kernel_arg_properties_t args[] = {
        {UR_STRUCTURE_TYPE_EXP_KERNEL_ARG_PROPERTIES, nullptr, UR_EXP_KERNEL_ARG_TYPE_POINTER, 0, sizeof(tmpBuffer), val}};

    constexpr size_t globalOffset[] = {0};
    size_t globalSize[] = {arguments.size / sizeof(int)};
    ASSERT_UR_RESULT_SUCCESS(urEnqueueKernelLaunchWithArgsExp(queue, kernel, 1, globalOffset, globalSize, nullptr, 1, args, nullptr, 0, nullptr, nullptr));

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

    if (arguments.strategy == TmpMemoryStrategy::Async) {
        ur_bool_t usmPoolSupport = false;
        auto status = urDeviceGetInfo(ur.device, UR_DEVICE_INFO_ASYNC_USM_ALLOCATIONS_SUPPORT_EXP,
                                      sizeof(usmPoolSupport), &usmPoolSupport, nullptr);

        if (status != UR_RESULT_SUCCESS || !usmPoolSupport) {
            return TestResult::DeviceNotCapable;
        }
    }

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
