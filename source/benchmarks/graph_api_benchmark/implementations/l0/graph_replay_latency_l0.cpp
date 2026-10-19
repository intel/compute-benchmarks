/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/graph_replay_latency.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const GraphReplayLatencyArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    const ExtensionProperties extensionProperties = ExtensionProperties::create().setGraphFunctions(true);
    LevelZero levelzero(QueueProperties::create().disable(), ContextProperties::create(), extensionProperties);
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::DeviceNotCapable;
    }

    const uint32_t replayCount = static_cast<uint32_t>(arguments.replayCount);
    const uint32_t forkCount = static_cast<uint32_t>(arguments.forkCount);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }
    constexpr int kernelOperationsCount = 20;
    const ze_group_count_t groupCount{1u, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));

    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));
    ze_command_list_handle_t captureCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &captureCmdList));
    std::vector<ze_command_list_handle_t> forkCmdLists(forkCount);
    for (auto &forkCmdList : forkCmdLists) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &forkCmdList));
    }

    const ze_event_counter_based_desc_t graphEventDesc{.stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC, .pNext = nullptr, .flags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE, .signal = 0, .wait = 0};
    ze_event_handle_t forkEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &graphEventDesc, &forkEvent));
    std::vector<ze_event_handle_t> joinEvents(forkCount);
    for (auto &joinEvent : joinEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &graphEventDesc, &joinEvent));
    }

    ze_graph_handle_t graph{};
    ze_executable_graph_handle_t execGraph{};
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphCreate(levelzero.context, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListBeginCaptureIntoGraph(captureCmdList, graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(captureCmdList, kernel, &groupCount, forkCount > 0 ? forkEvent : nullptr, 0, nullptr));
    for (auto i = 0u; i < forkCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(forkCmdLists[i], kernel, &groupCount, joinEvents[i], 1, &forkEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(captureCmdList, kernel, &groupCount, nullptr, forkCount, joinEvents.data()));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListEndGraphCapture(captureCmdList, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListInstantiateGraph(graph, &execGraph, nullptr));

    const ze_event_counter_based_desc_t profilingEventDesc{.stype = ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC,
                                                           .pNext = nullptr,
                                                           .flags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE | ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE | ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP,
                                                           .signal = ZE_EVENT_SCOPE_FLAG_HOST,
                                                           .wait = 0};
    std::vector<ze_event_handle_t> profilingEvents(replayCount);
    for (auto &profilingEvent : profilingEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &profilingEventDesc, &profilingEvent));
    }

    for (auto iteration = 0u; iteration < arguments.iterations; ++iteration) {
        for (auto j = 0u; j < replayCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(levelzero.graphExtension.commandListAppendGraph(cmdList, execGraph, nullptr, profilingEvents[j], 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

        auto switchTime = std::chrono::nanoseconds(0u);
        for (auto j = 1u; j < replayCount; j++) {
            const auto graphGap = levelzero.getAbsoluteTimeBetweenTwoKernels(profilingEvents[j - 1], profilingEvents[j]);
            if (!graphGap.has_value()) {
                return TestResult::Error;
            }
            switchTime += graphGap.value();
        }
        statistics.pushValue(switchTime / (replayCount - 1), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto &event : profilingEvents) {
        EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.executableGraphDestroy(execGraph));
    EXPECT_ZE_RESULT_SUCCESS(levelzero.graphExtension.graphDestroy(graph));
    for (auto &event : joinEvents) {
        EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    EXPECT_ZE_RESULT_SUCCESS(zeEventDestroy(forkEvent));
    for (auto &forkCmdList : forkCmdLists) {
        EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(forkCmdList));
    }
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(captureCmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    EXPECT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    EXPECT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<GraphReplayLatency> registerTestCase(run, Api::L0);
