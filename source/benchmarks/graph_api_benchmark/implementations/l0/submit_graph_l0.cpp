/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"
#include "framework/utility/file_helper.h"

#include "definitions/submit_graph.h"

#include <iostream>

static TestResult run([[maybe_unused]] const SubmitGraphArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats prof(Configuration::get().profilerType);
    if (arguments.useHostTasks || arguments.useExplicit) {
        return TestResult::ApiNotCapable;
    } else if (isNoopRun()) {
        prof.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setCounterBasedCreateFunctions(arguments.inOrderQueue)
                                                  .setGraphFunctions(!arguments.emulateGraphs);
    LevelZero levelzero(extensionProperties);

    const ze_group_count_t groupCount{1, 1, 1};

    // Only used when emulateGraphs is 0
    ze_graph_handle_t graph{};
    ze_executable_graph_handle_t execGraph{};

    if (!arguments.emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphCreate(levelzero.context, &graph, nullptr));
    }

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

    // Create command list that represents command graph
    ze_command_list_handle_t graphCmdList{};

    ze_command_list_desc_t cmdListDesc = {ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};

    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    cmdListDesc.flags = arguments.inOrderQueue ? ZE_COMMAND_LIST_FLAG_IN_ORDER : 0;

    if (!arguments.emulateGraphs) {
        ze_command_queue_desc_t capturedQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
        capturedQueueDesc.flags = arguments.inOrderQueue ? ZE_COMMAND_QUEUE_FLAG_IN_ORDER : 0;
        capturedQueueDesc.ordinal = levelzero.commandQueueDesc.ordinal;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(
            levelzero.context, levelzero.device, &capturedQueueDesc, &graphCmdList));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(
            levelzero.context, levelzero.device, &cmdListDesc, &graphCmdList));
    }

    // Configure kernel
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    // Begin recording stage
    if (!arguments.emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListBeginCaptureIntoGraph(graphCmdList, graph, nullptr));
    }

    for (uint32_t i = 0; i < arguments.numKernels; ++i) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(
            graphCmdList, kernel, &groupCount, nullptr, 0, nullptr));
    }

    // End recording stage
    if (!arguments.emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListEndGraphCapture(graphCmdList, &graph, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListInstantiateGraph(graph, &execGraph, nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(graphCmdList));
    }

    // cb event description
    const bool counterBasedEvents = arguments.inOrderQueue;
    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
    counterBasedEventDesc.flags |= arguments.useProfiling ? ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP : 0;
    counterBasedEventDesc.signalScope = ZE_EVENT_SCOPE_FLAG_DEVICE;
    counterBasedEventDesc.waitScope = ZE_EVENT_SCOPE_FLAG_HOST;

    // Create event pool (if not using counter based events)
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.flags |= arguments.useProfiling ? ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP : 0;
    eventPoolDesc.count = static_cast<uint32_t>(arguments.numKernels); // ensures one unique event per kernel

    ze_event_pool_handle_t eventPool = nullptr;
    if (!counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    }

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ze_event_handle_t event{};
    ze_event_handle_t signalEvent = nullptr;

    // warmup
    if (arguments.useEvents) {
        if (counterBasedEvents) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(
                levelzero.context, levelzero.device, &counterBasedEventDesc, &event));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
        }
        signalEvent = event;
    }

    if (!arguments.emulateGraphs) {
        ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(
            cmdList, execGraph, nullptr, signalEvent, 0, nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
            cmdList, 1, &graphCmdList, signalEvent, 0, nullptr));
    }

    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(
            signalEvent, std::numeric_limits<uint64_t>::max()));
        if (!counterBasedEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
            cmdList, std::numeric_limits<uint64_t>::max()));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        prof.measureStart();

        if (!arguments.emulateGraphs) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(
                cmdList, execGraph, nullptr, signalEvent, 0, nullptr));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListImmediateAppendCommandListsExp(
                cmdList, 1, &graphCmdList, signalEvent, 0, nullptr));
        }

        if (!arguments.measureCompletionTime) {
            prof.measureEnd();
        }

        if (arguments.useEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(
                signalEvent, std::numeric_limits<uint64_t>::max()));
            if (!counterBasedEvents) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
            }
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(
                cmdList, std::numeric_limits<uint64_t>::max()));
        }

        if (arguments.measureCompletionTime) {
            prof.measureEnd();
        }
        prof.pushStats(statistics);
    }

    // Cleanup
    if (arguments.useEvents) {
        EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        if (!counterBasedEvents) {
            EXPECT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
        }
    }

    // Cleanup graph objects if used
    if (!arguments.emulateGraphs) {
        EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.executableGraphDestroy(execGraph));
        EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphDestroy(graph));
    }

    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(graphCmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitGraph> registerTestCase(run, Api::L0);
