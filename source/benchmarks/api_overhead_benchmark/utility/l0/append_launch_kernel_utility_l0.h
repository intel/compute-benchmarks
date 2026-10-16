/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include "definitions/append_launch_kernel.h"

#include <level_zero/ze_api.h>
#include <vector>

template <typename AppendKernel>
TestResult measureAppendLaunchKernel(const AppendLaunchKernelArguments &arguments, Statistics &statistics, bool useCooperativeKernel, const AppendKernel &appendKernel) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0::LevelZero levelzero;
    Timer timer;

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    const char *const kernelFile = useCooperativeKernel ? "api_overhead_benchmark_cooperative_kernel.cl" : "api_overhead_benchmark_empty_kernel.cl";
    const char *const kernelName = useCooperativeKernel ? "cooperative_kernel" : "empty";
    if (auto result = L0::KernelHelper::loadKernel(levelzero, kernelFile, kernelName, &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    const uint32_t groupSizeX = static_cast<uint32_t>(arguments.workgroupSize);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, 1u, 1u));

    void *buffer = nullptr;
    if (useCooperativeKernel) {
        uint32_t maxCooperativeGroupCount = 0u;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestMaxCooperativeGroupCount(kernel, &maxCooperativeGroupCount));
        if (arguments.workgroupCount > maxCooperativeGroupCount) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
            ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
            return TestResult::DeviceNotCapable;
        }

        const size_t bufferSize = arguments.workgroupCount * sizeof(int);
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, bufferSize, &buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));
    }

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

    const ze_group_count_t dispatchTraits{static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    for (auto i = 0u; i < arguments.iterations; ++i) {
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

        timer.measureStart();
        for (auto j = 0u; j < arguments.appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(appendKernel(cmdList, kernel, &dispatchTraits, events[j]));
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
    if (useCooperativeKernel) {
        ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, buffer));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}
