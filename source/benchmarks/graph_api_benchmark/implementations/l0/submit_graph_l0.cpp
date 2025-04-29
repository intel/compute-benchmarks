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

#include "definitions/submit_graph.h"

#include <iostream>

static TestResult run([[maybe_unused]] const SubmitGraphArguments &arguments, Statistics &statistics) {

    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(
        arguments.inOrderQueue);
    LevelZero levelzero(extensionProperties);

    Timer timer;
    const ze_group_count_t groupCount{1, 1, 1};

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.size() == 0)
        return TestResult::KernelNotFound;
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Create an immediate command list
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.inOrderQueue) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Create a regular command list that represents command graph
    ze_command_list_handle_t graphCmdList{};

    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};

    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    cmdListDesc.flags = arguments.inOrderQueue ? ZE_COMMAND_LIST_FLAG_IN_ORDER : 0;

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(
        levelzero.context, levelzero.device, &cmdListDesc, &graphCmdList));

    // Configure kernel
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    // Append kernel to graph command list
    for (uint32_t i = 0; i < arguments.numKernels; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
            graphCmdList, kernel, &groupCount, nullptr, 0, nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(graphCmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
        cmdList, 1, &graphCmdList, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
        cmdList, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
            cmdList, 1, &graphCmdList, nullptr, 0, nullptr));

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
            cmdList, std::numeric_limits<uint64_t>::max()));

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(graphCmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitGraph> registerTestCase(run, Api::L0);
