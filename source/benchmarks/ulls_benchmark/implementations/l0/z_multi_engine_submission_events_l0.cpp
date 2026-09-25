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

#include "definitions/z_multi_engine_submission_events.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <string>

namespace {

struct EngineTarget {
    ze_command_queue_desc_t desc{};
    std::string label{};
    bool isCopyEngine{};
    ze_command_list_handle_t list{};
    ze_event_handle_t event{};
};

} // namespace

static TestResult run(const ZMultiEngineSubmissionEventsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{QueueProperties::create().disable()};
    constexpr static auto copySize = 4096u;

    std::vector<EngineTarget> engines{};
    for (const size_t index : arguments.ccsMask.getEnabledBits()) {
        const auto engine = EngineHelper::getComputeEngineFromIndex(index);
        const auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, engine);
        if (queueDesc != nullptr) {
            engines.push_back({queueDesc->desc, EngineHelper::getEngineName(engine), false});
        }
    }

    for (const size_t index : arguments.bcsMask.getEnabledBits()) {
        const auto engine = EngineHelper::getBlitterEngineFromIndex(index);
        const auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, engine);
        if (queueDesc != nullptr) {
            engines.push_back({queueDesc->desc, EngineHelper::getEngineName(engine), true});
        }
    }

    if (arguments.useHpCopyEngine) {
        const auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Bcs);
        if (queueDesc != nullptr) {
            auto desc = queueDesc->desc;
            desc.priority = QueueProperties::create().setPriority(PriorityLevel::High).priority;
            engines.push_back({desc, "BCS-HP", true});
        }
    }

    if (engines.empty()) {
        return TestResult::InvalidArgs;
    }

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    const auto kernelLoadResult = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_empty_kernel.cl", "empty", &kernel, &module, nullptr);
    if (kernelLoadResult != TestResult::Success) {
        return kernelLoadResult;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));

    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *source{};
    void *destination{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, copySize, 0, levelzero.device, &source));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, copySize, 0, levelzero.device, &destination));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero.device, levelzero.context, nullptr, engines.front().desc.ordinal,
                                                                source, copySize, BufferContents::Random, true));

    const auto engineCount = static_cast<uint32_t>(engines.size());
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, engineCount};
    ze_event_pool_handle_t eventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    for (auto engineIndex = 0u; engineIndex < engineCount; engineIndex++) {
        auto &engine = engines[engineIndex];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &engine.desc, &engine.list));

        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
        eventDesc.index = engineIndex;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &engine.event));
    }

    const ze_group_count_t groupCount{1u, 1u, 1u};
    std::vector<Timer::Clock::duration> hostOffsets(engineCount);
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        uint64_t hostEnqueueTimestamp{};
        uint64_t deviceEnqueueTimestamp{};
        ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetGlobalTimestamps(levelzero.device, &hostEnqueueTimestamp, &deviceEnqueueTimestamp));

        const auto burstStart = Timer::Clock::now();

        for (auto engineIndex = 0u; engineIndex < engineCount; engineIndex++) {
            const auto &engine = engines[engineIndex];
            hostOffsets[engineIndex] = Timer::Clock::now() - burstStart;
            if (engine.isCopyEngine) {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(engine.list, destination, source, copySize, engine.event, 0, nullptr));
            } else {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(engine.list, kernel, &groupCount, engine.event, 0, nullptr));
            }
        }
        for (const auto &engine : engines) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(engine.event, std::numeric_limits<uint64_t>::max()));
        }

        std::chrono::nanoseconds worstSubmissionTime{};
        for (auto engineIndex = 0u; engineIndex < engineCount; engineIndex++) {
            const auto &engine = engines[engineIndex];
            ze_kernel_timestamp_result_t timestamp{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(engine.event, &timestamp));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(engine.event));
            const auto rawSubmissionTime = levelzero.getAbsoluteSubmissionTime(timestamp.global, deviceEnqueueTimestamp);

            const auto hostOffset = std::chrono::duration_cast<std::chrono::nanoseconds>(hostOffsets[engineIndex]);
            const auto submissionTime = (rawSubmissionTime > hostOffset) ? (rawSubmissionTime - hostOffset) : std::chrono::nanoseconds::zero();

            statistics.pushValue(submissionTime, typeSelector.getUnit(), typeSelector.getType(), engine.label);
            worstSubmissionTime = std::max(worstSubmissionTime, submissionTime);
        }

        statistics.pushValue(worstSubmissionTime, typeSelector.getUnit(), typeSelector.getType(), "Worst");
    }

    for (const auto &engine : engines) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(engine.event));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(engine.list));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, source));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, destination));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ZMultiEngineSubmissionEvents> registerTestCase(run, Api::L0);
