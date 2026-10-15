/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/command_list_update_mutable_command_signal_event.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const CommandListUpdateMutableCommandSignalEventArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

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

    if (!(levelzero.getDeviceMclProperties().mutableCommandFlags & ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT)) {
        return TestResult::DeviceNotCapable;
    }

    ze_command_list_handle_t mutableCmdList = nullptr;
    ze_mutable_command_list_exp_desc_t mutableCmdListExpDesc{
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_LIST_EXP_DESC, nullptr, 0};
    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC,
                                          &mutableCmdListExpDesc, 0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &mutableCmdList));

    ze_kernel_handle_t kernel = nullptr;
    ze_module_handle_t module = nullptr;
    auto kernelLoadRes = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernel, &module, nullptr);
    if (kernelLoadRes != TestResult::Success) {
        return kernelLoadRes;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    const int initialKernelOpsCount = 1;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &initialKernelOpsCount));

    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 2;
    ze_event_pool_handle_t eventPool = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    ze_event_handle_t events[2] = {nullptr, nullptr};
    for (uint32_t i = 0; i < 2; ++i) {
        eventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[i]));
    }

    ze_mutable_command_id_exp_desc_t commandIdDesc = {
        ZE_STRUCTURE_TYPE_MUTABLE_COMMAND_ID_EXP_DESC, nullptr,
        ZE_MUTABLE_COMMAND_EXP_FLAG_SIGNAL_EVENT};
    ze_group_count_t initialGroupCount = {1u, 1u, 1u};

    uint64_t commandId = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListGetNextCommandIdExp(mutableCmdList, &commandIdDesc, &commandId));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(mutableCmdList, kernel,
                                                             &initialGroupCount, events[0], 0, nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(mutableCmdList));

    for (auto j = 0u; j < arguments.iterations; j++) {
        ze_event_handle_t newSignalEvent = events[j & 1u];

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListUpdateMutableCommandSignalEventExp(mutableCmdList, commandId, newSignalEvent));
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto event : events) {
        if (event) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
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

static RegisterTestCaseImplementation<CommandListUpdateMutableCommandSignalEvent> registerTestCase(run, Api::L0);
