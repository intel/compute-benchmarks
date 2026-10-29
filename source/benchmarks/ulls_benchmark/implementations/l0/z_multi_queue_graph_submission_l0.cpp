/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/z_multi_queue_graph_submission.h"

#include <gtest/gtest.h>
#include <string>

using namespace L0;

namespace {

struct EngineTarget {
    ze_command_queue_desc_t desc{};
    std::string label{};
    bool isCopyEngine{};
    ze_command_list_handle_t list{};
    ze_event_handle_t joinEvent{};
    void *destination{};
};

} // namespace

static TestResult run(const ZMultiQueueGraphSubmissionArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);
    const MeasurementFields cpuTypeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Probed before initializeExtension, which aborts on a missing entrypoint instead of skipping.
    LevelZero levelzero{QueueProperties::create().disable()};
    if (!levelzero.isExtensionSupported(ZE_RECORD_REPLAY_GRAPH_EXP_NAME)) {
        return TestResult::ApiNotCapable;
    }
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }
    levelzero.initializeExtension(ExtensionProperties::create().setGraphFunctions(true));
    const auto &graphApi = levelzero.graphExtension;
    constexpr static auto copySize = 4096u;

    auto rootQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    if (rootQueueDesc == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    rootQueueDesc->desc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;

    std::vector<EngineTarget> engines{};
    for (const auto &selectedEngine : EngineBitmaskHelper::getSelectedEngines(arguments.ccsMask, arguments.bcsMask, arguments.useHpCopyEngine)) {
        const auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, selectedEngine.engine);
        if (queueDesc == nullptr) {
            continue;
        }
        engines.push_back({queueDesc->desc, selectedEngine.label, selectedEngine.isCopyEngine});
        engines.back().desc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
        if (selectedEngine.isHighPriority) {
            engines.back().desc.priority = QueueProperties::create().setPriority(PriorityLevel::High).priority;
        }
    }
    if (engines.empty()) {
        return TestResult::DeviceNotCapable;
    }

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    const auto kernelLoadResult = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_empty_kernel.cl", "empty", &kernel, &module, nullptr);
    if (kernelLoadResult != TestResult::Success) {
        return kernelLoadResult;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    ze_command_list_handle_t rootList{};
    ze_command_list_handle_t replayList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &rootQueueDesc->desc, &rootList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &rootQueueDesc->desc, &replayList));

    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *source{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, copySize, 0, levelzero.device, &source));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero.device, levelzero.context, nullptr, rootQueueDesc->desc.ordinal,
                                                                source, copySize, BufferContents::Random, true));

    const auto engineCount = static_cast<uint32_t>(engines.size());
    const ze_host_mem_alloc_desc_t hostAllocDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    uint64_t *engineStartTimestamps{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &hostAllocDesc, (engineCount + 1) * sizeof(uint64_t), 0, reinterpret_cast<void **>(&engineStartTimestamps)));
    uint64_t *const rootStartTimestamp = &engineStartTimestamps[engineCount];

    const ze_event_counter_based_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC, nullptr, ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE,
                                                  ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_handle_t forkEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &forkEvent));

    std::vector<ze_event_handle_t> joinEvents{};
    for (auto &engine : engines) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &engine.desc, &engine.list));
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &engine.joinEvent));
        joinEvents.push_back(engine.joinEvent);
        if (engine.isCopyEngine) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, copySize, 0, levelzero.device, &engine.destination));
        }
    }

    const ze_group_count_t groupCount{1u, 1u, 1u};
    ze_graph_handle_t graph{};
    ASSERT_ZE_RESULT_SUCCESS(graphApi.graphCreate(levelzero.context, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListBeginCaptureIntoGraph(rootList, graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(rootList, rootStartTimestamp, forkEvent, 0, nullptr));
    for (auto engineIndex = 0u; engineIndex < engineCount; engineIndex++) {
        const auto &engine = engines[engineIndex];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendWriteGlobalTimestamp(engine.list, &engineStartTimestamps[engineIndex], nullptr, 1, &forkEvent));
        if (engine.isCopyEngine) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(engine.list, engine.destination, source, copySize, engine.joinEvent, 0, nullptr));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(engine.list, kernel, &groupCount, engine.joinEvent, 0, nullptr));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(rootList, nullptr, static_cast<uint32_t>(joinEvents.size()), joinEvents.data()));
    ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListEndGraphCapture(rootList, &graph, nullptr));

    ze_executable_graph_handle_t executableGraph{};
    ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListInstantiateGraph(graph, &executableGraph, nullptr));

    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const uint64_t timestampValidBitsMask = levelzero.getTimestampValidBitsMask(levelzero.device);
    const auto getElapsedTime = [&](const uint64_t startTimestamp, const uint64_t endTimestamp) {
        return levelzero.getAbsoluteTimestampTime(startTimestamp & timestampValidBitsMask, endTimestamp & timestampValidBitsMask,
                                                  timerResolution, timestampValidBitsMask);
    };

    Timer submitTimer;
    Timer totalTimer;
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        uint64_t hostEnqueueTimestamp{};
        uint64_t deviceEnqueueTimestamp{};
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetGlobalTimestamps(levelzero.device, &hostEnqueueTimestamp, &deviceEnqueueTimestamp));
        totalTimer.measureStart();
        submitTimer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(graphApi.commandListAppendGraph(replayList, executableGraph, nullptr, nullptr, 0, nullptr));
        submitTimer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(replayList, std::numeric_limits<uint64_t>::max()));
        totalTimer.measureEnd();

        statistics.pushValue(getElapsedTime(deviceEnqueueTimestamp, *rootStartTimestamp), typeSelector.getUnit(), typeSelector.getType(), "Root");
        std::chrono::nanoseconds worstSubmissionTime{};
        for (auto engineIndex = 0u; engineIndex < engineCount; engineIndex++) {
            const auto forkTime = getElapsedTime(*rootStartTimestamp, engineStartTimestamps[engineIndex]);
            statistics.pushValue(forkTime, typeSelector.getUnit(), typeSelector.getType(), engines[engineIndex].label + " fork");
            worstSubmissionTime = std::max(worstSubmissionTime, getElapsedTime(deviceEnqueueTimestamp, engineStartTimestamps[engineIndex]));
        }
        statistics.pushValue(worstSubmissionTime, typeSelector.getUnit(), typeSelector.getType(), "Worst");
        statistics.pushValue(submitTimer.get(), cpuTypeSelector.getUnit(), cpuTypeSelector.getType(), "Submit (Cpu)");
        statistics.pushValue(totalTimer.get(), cpuTypeSelector.getUnit(), cpuTypeSelector.getType(), "Total (Cpu)");
    }

    ASSERT_ZE_RESULT_SUCCESS(graphApi.executableGraphDestroy(executableGraph));
    ASSERT_ZE_RESULT_SUCCESS(graphApi.graphDestroy(graph));
    for (const auto &engine : engines) {
        if (engine.destination != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, engine.destination));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(engine.joinEvent));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(engine.list));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(forkEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, engineStartTimestamps));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, source));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(replayList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(rootList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ZMultiQueueGraphSubmission> registerTestCase(run, Api::L0, true);
