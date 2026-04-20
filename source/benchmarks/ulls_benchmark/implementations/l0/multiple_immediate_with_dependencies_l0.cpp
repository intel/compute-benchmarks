/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/multiple_immediate_with_dependencies.h"

#include <gtest/gtest.h>

static TestResult runSingleIteration(const MultipleImmediateCmdListsWithDependenciesArguments &arguments, const std::vector<ze_command_list_handle_t> &cmdLists,
                                     const std::vector<ze_kernel_handle_t> &kernels, const std::vector<ze_event_handle_t> &events) {
    const ze_group_count_t groupCount{1, 1, 1};
    const uint32_t cmdlistCount = static_cast<uint32_t>(arguments.cmdlistCount);
    const uint32_t submissionsPerQueue = static_cast<uint32_t>(arguments.submissionsPerQueue);

    for (uint32_t i = 0u; i < cmdlistCount; i++) {
        for (uint32_t j = 0u; j < submissionsPerQueue; j++) {
            ze_event_handle_t signalEvent = (j == submissionsPerQueue - 1u) ? events[i] : nullptr;
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, signalEvent, 0, nullptr));
        }
    }

    if (arguments.useEventForHostSync) {
        for (uint32_t i = 0u; i < cmdlistCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[i], std::numeric_limits<uint64_t>::max()));
        }
    } else {
        for (uint32_t i = 0u; i < cmdlistCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdLists[i], std::numeric_limits<uint64_t>::max()));
        }
    }

    return TestResult::Success;
}

static TestResult run(const MultipleImmediateCmdListsWithDependenciesArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{QueueProperties::create().disable()};
    Timer timer;

    const uint32_t cmdlistCount = static_cast<uint32_t>(arguments.cmdlistCount);

    std::vector<ze_event_handle_t> events(cmdlistCount);
    for (auto i = 0u; i < cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &defaultIntelCounterBasedEventDesc, &events[i]));
    }

    const auto kernelBinary = FileHelper::loadBinaryFile("ulls_benchmark_eat_time.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "eat_time";
    std::vector<ze_kernel_handle_t> kernels(cmdlistCount);
    for (auto i = 0u; i < cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1, 1, 1));
        int32_t time = 100;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(time), &time));
    }

    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    commandQueueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    std::vector<ze_command_list_handle_t> cmdLists(cmdlistCount);
    for (auto i = 0u; i < cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdLists[i]));
    }

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        timer.measureStart();
        auto result = runSingleIteration(arguments, cmdLists, kernels, events);
        if (result != TestResult::Success) {
            return result;
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto i = 0u; i < cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdLists[i]));
    }
    for (auto i = 0u; i < cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultipleImmediateCmdListsWithDependencies> registerTestCase(run, Api::L0);
