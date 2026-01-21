/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/append_launch_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const AppendLaunchKernelArguments &arguments, Statistics &statistics) {
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

    // Configure kernel
    uint32_t groupSizeX = static_cast<uint32_t>(arguments.workgroupSize);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, 1u, 1u));

    // Create event if necessary
    const ze_event_pool_desc_t eventPoolDesc = {ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, arguments.appendCount};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(arguments.appendCount);
    if (arguments.useEvent) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    // Create command list descriptor
    const ze_group_count_t dispatchTraits{static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; ++i) {
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        timer.measureStart();
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, events[j], 0, nullptr));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / arguments.appendCount, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    }

    if (arguments.useEvent) {
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendLaunchKernel> registerTestCase(run, Api::L0);
