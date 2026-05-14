/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/append_kernel_with_mapped_timestamp_event.h"

#include <gtest/gtest.h>

static TestResult run(const AppendKernelWithMappedTimestampEventArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_empty_kernel.spv");
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
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    const ze_group_count_t dispatchTraits{1u, 1u, 1u};

    // Create event pool with benchmarked flag
    ze_event_pool_flags_t poolFlags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    poolFlags |= arguments.useMappedTimestampEvent
                     ? ZE_EVENT_POOL_FLAG_KERNEL_MAPPED_TIMESTAMP
                     : ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = poolFlags;
    eventPoolDesc.count = 1; // one slot reused across iterations (destroy before next create)
    ze_event_pool_handle_t eventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.index = 0;
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_DEVICE;

    // Create immediate command list
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; ++i) {
        ze_event_handle_t event{};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendKernelWithMappedTimestampEvent> registerTestCase(run, Api::L0);
