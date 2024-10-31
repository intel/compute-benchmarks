/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/multi_kernel_execution.h"

#include <gtest/gtest.h>
#include <level_zero/zex_event.h>
using L0EventGetDeviceAddress = decltype(&zexEventGetDeviceAddress);

static TestResult run(const MultiKernelExecutionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;

    const size_t lws = arguments.workgroupSize;
    const size_t gws = lws * arguments.workgroupCount;

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("ulls_benchmark_multi_kernel_execution.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = arguments.inOrderOverOOO ? "emptyWithSynchro" : "empty";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));

    // Create command list and append kernel
    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    if (!arguments.inOrderOverOOO) {
        cmdListDesc.flags = ZE_COMMAND_LIST_FLAG_IN_ORDER;
    }

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    Timer timer;

    if (!arguments.inOrderOverOOO) {
        const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
        const auto timestampBufferSize = sizeof(uint64_t) * 2;
        void *timestampBuffer = nullptr;
        uint64_t *beginTimestamp = nullptr;
        uint64_t *endTimestamp = nullptr;
        const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);

        if (arguments.profiling) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocationDesc, timestampBufferSize, 0, &timestampBuffer));
            ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, timestampBuffer, timestampBufferSize))
            beginTimestamp = static_cast<uint64_t *>(timestampBuffer);
            endTimestamp = beginTimestamp + 1;
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, beginTimestamp, nullptr, 0, nullptr));
        }

        for (auto i = 0u; i < arguments.kernelCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }

        if (arguments.profiling) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(cmdList, endTimestamp, nullptr, 0, nullptr));
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

        // Warmup
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        // Benchmark
        for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            if (arguments.profiling) {
                auto totalTime = std::chrono::nanoseconds(*endTimestamp - *beginTimestamp);
                totalTime *= timerResolution;
                statistics.pushValue(totalTime, typeSelector.getUnit(), MeasurementType::Gpu);
            } else {
                statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
            }
        }
        if (arguments.profiling) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, timestampBuffer));
        }
    } else {

        L0EventGetDeviceAddress eventGetAddressFunc = nullptr;
        EXPECT_ZE_RESULT_SUCCESS(
            zeDriverGetExtensionFunctionAddress(levelzero.driver,
                                                "zexEventGetDeviceAddress",
                                                reinterpret_cast<void **>(&eventGetAddressFunc)));
        FATAL_ERROR_IF(eventGetAddressFunc == nullptr, "zexEventGetDeviceAddress retrieved nullptr");

        ze_event_pool_flags_t flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, flags, static_cast<uint32_t>(arguments.kernelCount)};
        uint32_t numDevices = 1;
        ze_event_pool_handle_t hEventPool;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, numDevices, &levelzero.device, &hEventPool));

        std::vector<ze_event_handle_t> events(arguments.kernelCount);
        for (auto i = 0u; i < arguments.kernelCount; i++) {
            ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, i, 0, 0};
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(hEventPool, &eventDesc, &events[i]));
        }

        uint64_t eventAddress = 0llu;
        uint64_t completionValue = 0llu;
        uint32_t delayValue = (uint32_t)arguments.delay;

        for (auto i = 0u; i < arguments.kernelCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(eventAddress), &eventAddress));
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(completionValue), &completionValue));
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(delayValue), &delayValue));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, events[i], 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(eventGetAddressFunc(events[i], &completionValue, &eventAddress));
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

        // Warmup
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        for (auto i = 0u; i < arguments.kernelCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
        }

        // Benchmark
        for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
            for (auto i = 0u; i < arguments.kernelCount; i++) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
            }
        }
        for (auto &hEvent : events) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(hEvent));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(hEventPool));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiKernelExecution> registerTestCase(run, Api::L0);
