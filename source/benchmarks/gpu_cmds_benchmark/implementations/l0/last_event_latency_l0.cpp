/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/last_event_latency.h"

#include <gtest/gtest.h>

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
    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setCounterBasedCreateFunctions(true)
                                                  .setSimplifiedL0Functions(true);
    LevelZero levelzero(QueueProperties::create().disable(), ContextProperties::create().disable(), extensionProperties);
    auto appendLaunchKernelWithArgumentsFunc = levelzero.zeCommandListAppendLaunchKernelWithArguments;
    auto context = levelzero.zeDriverGetDefaultContext(levelzero.driver);

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("gpu_cmds_benchmark_write_one_global_ids.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "write_one_with_args";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Setup kernel args and properties
    const ze_group_count_t wgc{32u, 1u, 1u};
    const ze_group_size_t wgs{32u, 1u, 1u};
    auto buffSize = wgc.groupCountX * wgs.groupSizeX * sizeof(uint32_t);
    void *deviceUsmPtr = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(context, &defaultDeviceMemDesc, buffSize, 0, levelzero.device, &deviceUsmPtr));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(context, levelzero.device, deviceUsmPtr, buffSize))
    size_t slmSize = 1024;
    uint32_t immData = 10u;
    void *kernelArgs[3] = {&deviceUsmPtr, &slmSize, &immData};

    // Setup command list with CB event
    ze_command_list_handle_t cmdList;
    ze_event_handle_t event;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(context, levelzero.device, &defaultCommandQueueDesc, &cmdList));
    levelzero.counterBasedEventCreate2(context, levelzero.device, &defaultCounterBasedEventDesc, &event);
    auto eventOnKernel = !arguments.signalOnBarrier ? event : nullptr;
    ze_command_list_handle_t barrierCmdList = cmdList;
    auto dependencyOnKernel = arguments.useSameCmdList ? 0 : 1;
    if (!arguments.useSameCmdList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(context, levelzero.device, &defaultCommandQueueDesc, &barrierCmdList));
        levelzero.counterBasedEventCreate2(context, levelzero.device, &defaultCounterBasedEventDesc, &eventOnKernel);
    }

    // Warmup
    auto ret = appendLaunchKernelWithArgumentsFunc(cmdList, kernel, wgc, wgs, kernelArgs, nullptr, eventOnKernel, 0, nullptr);
    ASSERT_ZE_RESULT_SUCCESS(ret);
    if (arguments.signalOnBarrier) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(barrierCmdList, event, dependencyOnKernel, &eventOnKernel));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        timer.measureStart();

        ret = appendLaunchKernelWithArgumentsFunc(cmdList, kernel, wgc, wgs, kernelArgs, nullptr, eventOnKernel, 0, nullptr);
        ASSERT_ZE_RESULT_SUCCESS(ret);
        if (arguments.signalOnBarrier) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(barrierCmdList, event, dependencyOnKernel, &eventOnKernel));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(context, deviceUsmPtr));
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
