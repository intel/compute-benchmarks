/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/multiple_immediate_with_dependencies.h"

#include <gtest/gtest.h>

static TestResult run(const MultipleImmediateCmdListsWithDependenciesArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{QueueProperties::create().disable()};
    Timer timer;

    uint32_t submissionsPerQueue = 4u;
    uint32_t totalEventCount = submissionsPerQueue * static_cast<uint32_t>(arguments.cmdlistCount);

    // Create events
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = totalEventCount;
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    std::vector<ze_event_handle_t> events(totalEventCount);
    for (auto i = 0u; i < totalEventCount; i++) {
        eventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[i]));
    }

    // Create and configure kernels
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
    std::vector<ze_kernel_handle_t> kernels(arguments.cmdlistCount);
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1, 1, 1));
        int32_t time = 10000;
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(time), &time));
    }

    // Create an immediate command lists
    const ze_group_count_t groupCount{1, 1, 1};
    std::vector<ze_command_list_handle_t> cmdLists(arguments.cmdlistCount);
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdLists[i]));
    }

    uint32_t eventIdentifier = 0u;

    // Warmup
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, events[eventIdentifier], 0, nullptr));
        eventIdentifier++;

        for (auto j = 1u; j < submissionsPerQueue; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, events[eventIdentifier], 1, &events[eventIdentifier - 1]));
            eventIdentifier++;
        }
    }
    for (auto i = 0u; i < totalEventCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[i], std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
    }

    // Benchmark
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        eventIdentifier = 0u;
        timer.measureStart();
        for (auto i = 0u; i < arguments.cmdlistCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, events[eventIdentifier], 0, nullptr));
            eventIdentifier++;

            for (auto j = 1u; j < submissionsPerQueue; j++) {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, events[eventIdentifier], 1, &events[eventIdentifier - 1]));
                eventIdentifier++;
            }
        }

        for (auto i = 0u; i < totalEventCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[i], std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();
        for (auto i = 0u; i < totalEventCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup

    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdLists[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultipleImmediateCmdListsWithDependencies> registerTestCase(run, Api::L0);
