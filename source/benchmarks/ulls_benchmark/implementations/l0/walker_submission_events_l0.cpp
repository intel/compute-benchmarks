/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/bit_operations_helper.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/walker_submission_events.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

using Clock = std::chrono::high_resolution_clock;

static TestResult run(const WalkerSubmissionEventsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const uint32_t timestampValidBits = levelzero.getTimestampValidBits(levelzero.device);
    const uint32_t kernelTimestampValidBits = levelzero.getKernelTimestampValidBits(levelzero.device);
    const uint32_t sharedTimestampValidBits = std::min(timestampValidBits, kernelTimestampValidBits);

    // Create event for profiling
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, 1};
    uint32_t numDevices = 1;
    ze_event_pool_handle_t hEventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));

    const ze_event_desc_t eventDesc{};
    ze_event_handle_t hEvent;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &hEvent));

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("ulls_benchmark_empty_kernel.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "empty";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Create command list
    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    const ze_group_count_t dispatchTraits{1u, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, hEvent, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    uint64_t hostEnqueueTimestamp = 0;
    uint64_t deviceEnqueueTimestamp = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetGlobalTimestamps(levelzero.device, &hostEnqueueTimestamp, &deviceEnqueueTimestamp));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, std::numeric_limits<uint64_t>::max()));

    ze_kernel_timestamp_result_t kernelTimestamp;
    ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(hEvent, &kernelTimestamp));

    uint64_t truncatedDeviceEnqueueTimestamp = BitHelper::isolateLowerNBits(deviceEnqueueTimestamp, sharedTimestampValidBits);
    uint64_t truncatedKernelStartTimestamp = BitHelper::isolateLowerNBits(kernelTimestamp.global.kernelStart, sharedTimestampValidBits);
    EXPECT_GT(truncatedKernelStartTimestamp, truncatedDeviceEnqueueTimestamp);

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(hEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetGlobalTimestamps(levelzero.device, &hostEnqueueTimestamp, &deviceEnqueueTimestamp));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(hEvent, std::numeric_limits<uint64_t>::max()));

        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(hEvent, &kernelTimestamp));
        truncatedDeviceEnqueueTimestamp = BitHelper::isolateLowerNBits(deviceEnqueueTimestamp, sharedTimestampValidBits);
        truncatedKernelStartTimestamp = BitHelper::isolateLowerNBits(kernelTimestamp.global.kernelStart, sharedTimestampValidBits);

        std::chrono::nanoseconds submissionTime = levelzero.getAbsoluteSubmissionTime(truncatedKernelStartTimestamp,
                                                                                      truncatedDeviceEnqueueTimestamp,
                                                                                      timerResolution);

        statistics.pushValue(submissionTime, typeSelector.getUnit(), typeSelector.getType());
    }
    // Cleanup

    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<WalkerSubmissionEvents> registerTestCase(run, Api::L0);
