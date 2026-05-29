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

#include "definitions/command_list_update_mutable_command_wait_events.h"

#include <gtest/gtest.h>
#include <vector>

static TestResult run(const CommandListUpdateMutableCommandWaitEventsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    if (!levelzero.isMclExtensionAvailable(1, 0)) {
        return TestResult::DeviceNotCapable;
    }

    if (!(levelzero.getDeviceMclProperties().mutableCommandFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS)) {
        return TestResult::DeviceNotCapable;
    }

    const uint32_t numWaitEvents = static_cast<uint32_t>(arguments.numWaitEvents);
    if (numWaitEvents == 0u) {
        return TestResult::InvalidArgs;
    }

    // Create mutable command list
    ze_command_list_handle_t mutableCmdList = nullptr;
    ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                          &mutableCmdListExpDesc, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mutableCmdList));

    // Create module + kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module = nullptr;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));

    ze_kernel_handle_t kernel = nullptr;
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    const int initialKernelOpsCount = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &initialKernelOpsCount));

    // Create event pool with 2*N slots: two parallel banks of N events that we alternate between.
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 2u * numWaitEvents;
    ze_event_pool_handle_t eventPool = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    std::vector<ze_event_handle_t> waitEventsA(numWaitEvents, nullptr);
    std::vector<ze_event_handle_t> waitEventsB(numWaitEvents, nullptr);
    for (uint32_t i = 0; i < numWaitEvents; ++i) {
        eventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &waitEventsA[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEventsA[i]));

        eventDesc.index = numWaitEvents + i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &waitEventsB[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSignal(waitEventsB[i]));
    }

    // Append a single launch with N wait events
    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr,
        ZE_MUTABLE_COMMAND_EXP_FLAG_WAIT_EVENTS};
    ze_group_count_t initialGroupCount = {1u, 1u, 1u};

    uint64_t commandId = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(mutableCmdList, &commandIdDesc, &commandId));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mutableCmdList, kernel, &initialGroupCount,
                                                             nullptr, numWaitEvents, waitEventsA.data()));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mutableCmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandWaitEventsExp(
        mutableCmdList, commandId, numWaitEvents, waitEventsB.data()));

    // Benchmark - swap between the two banks each iteration.
    for (auto j = 0u; j < arguments.iterations; j++) {
        ze_event_handle_t *bank = (j & 1u) ? waitEventsB.data() : waitEventsA.data();

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandWaitEventsExp(
            mutableCmdList, commandId, numWaitEvents, bank));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (auto e : waitEventsA) {
        if (e) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(e));
        }
    }
    for (auto e : waitEventsB) {
        if (e) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(e));
        }
    }
    if (eventPool) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    if (kernel) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    }
    if (module) {
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    }
    if (mutableCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(mutableCmdList));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<CommandListUpdateMutableCommandWaitEvents> registerTestCase(run, Api::L0);
