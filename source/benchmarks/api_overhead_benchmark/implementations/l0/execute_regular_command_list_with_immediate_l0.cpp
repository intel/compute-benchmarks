/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/execute_regular_commandlist_with_immediate.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteRegularCommandListWithImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (arguments.counterBasedEvents && !arguments.inOrder) {
        return TestResult::ApiNotCapable;
    }

    if (arguments.counterBasedEvents && !arguments.useEvent) {
        return TestResult::ApiNotCapable;
    }

    if (arguments.waitEvent && !arguments.useEvent) {
        return TestResult::ApiNotCapable;
    }

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

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    // Create output event if neccessary
    uint32_t numEvents = 2;
    std::vector<ze_event_handle_t> events(numEvents);
    ze_event_pool_handle_t eventPool{};
    if (arguments.useEvent) {
        ze_event_pool_flags_t flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        if (arguments.useProfiling) {
            flags |= ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP;
        }
        const ze_event_pool_counter_based_exp_desc_t counterBasedDesc{ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC, nullptr, ZE_EVENT_POOL_COUNTER_BASED_EXP_FLAG_IMMEDIATE};

        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, arguments.counterBasedEvents ? &counterBasedDesc : nullptr, flags, static_cast<uint32_t>(numEvents)};

        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

        for (auto i = 0u; i < numEvents; i++) {
            ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, i, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[i]));
        }
    }

    // Create immediate command list
    ze_command_list_handle_t immediateCmdlist;
    if (arguments.inOrder) {
        levelzero.commandQueueDesc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &levelzero.commandQueueDesc, &immediateCmdlist));

    // Create regular command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    if (arguments.inOrder) {
        cmdListDesc.flags |= ZE_COMMAND_LIST_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    const ze_group_count_t dispatchTraits{1u, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(immediateCmdlist, 1, &cmdList, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(immediateCmdlist, std::numeric_limits<uint64_t>::max()));

    if (arguments.waitEvent && !arguments.counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(events[0]));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(immediateCmdlist, 1, &cmdList, arguments.useEvent ? events[1] : nullptr,
                                                                             arguments.waitEvent && !arguments.counterBasedEvents ? 1 : 0,
                                                                             arguments.waitEvent && !arguments.counterBasedEvents ? &events[0] : nullptr));
        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        if (arguments.useEvent) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[1], std::numeric_limits<uint64_t>::max()));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(immediateCmdlist, std::numeric_limits<uint64_t>::max()));
        }
        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    if (arguments.useEvent) {
        for (auto &event : events) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(immediateCmdlist));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteRegularCommandListWithImmediate> registerTestCase(run, Api::L0);
