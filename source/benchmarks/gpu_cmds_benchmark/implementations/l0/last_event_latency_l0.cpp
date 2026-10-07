/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/last_event_latency.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>

static TestResult run([[maybe_unused]] const LastEventLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    if (!arguments.signalOnBarrier && !arguments.useSameCmdList) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    Timer timer;
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    LevelZero levelzero(QueueProperties::create().disable(), ContextProperties::create(), extensionProperties);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "gpu_cmds_benchmark_write_one_global_ids.cl", "write_one_with_args", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    const ze_group_count_t wgc{32u, 1u, 1u};
    const ze_group_size_t wgs{32u, 1u, 1u};
    auto buffSize = wgc.groupCountX * wgs.groupSizeX * sizeof(uint32_t);
    void *deviceUsmPtr = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, buffSize, 0, levelzero.device, &deviceUsmPtr));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, deviceUsmPtr, buffSize))
    size_t slmSize = 1024;
    uint32_t immData = 10u;
    void *kernelArgs[3] = {&deviceUsmPtr, &slmSize, &immData};

    ze_command_list_handle_t cmdList;
    ze_event_handle_t event;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &defaultIntelCounterBasedEventDesc, &event));
    auto eventOnKernel = !arguments.signalOnBarrier ? event : nullptr;
    ze_command_list_handle_t barrierCmdList = cmdList;
    auto dependencyOnKernel = arguments.useSameCmdList ? 0 : 1;
    if (!arguments.useSameCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &barrierCmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &defaultIntelCounterBasedEventDesc, &eventOnKernel));
    }

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        timer.measureStart();

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, wgc, wgs, kernelArgs, nullptr, eventOnKernel, 0, nullptr));
        if (arguments.signalOnBarrier) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(barrierCmdList, event, dependencyOnKernel, &eventOnKernel));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, deviceUsmPtr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (!arguments.useSameCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(barrierCmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(eventOnKernel));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<LastEventLatency> registerTestCase(run, Api::L0);
