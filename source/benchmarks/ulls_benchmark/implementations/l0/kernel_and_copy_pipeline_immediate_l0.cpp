/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_and_copy_pipeline_immediate.h"

#include <limits>
#include <vector>

using namespace L0;

struct PipelineResources {
    ze_command_list_handle_t computeList{};
    ze_command_list_handle_t copyList{};
    ze_kernel_handle_t kernel{};
    std::vector<ze_event_handle_t> kernelEvents{};
    std::vector<ze_event_handle_t> copyEvents{};
    void *source{};
    void *destination{};
};

static TestResult runSingleIteration(const KernelAndCopyPipelineImmediateArguments &arguments, const PipelineResources &resources) {
    const ze_group_count_t groupCount{1, 1, 1};

    for (auto stage = 0u; stage < arguments.stages; stage++) {
        ze_event_handle_t kernelEvent = resources.kernelEvents[stage];
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(resources.computeList, resources.kernel, &groupCount, kernelEvent, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(resources.copyList, resources.destination, resources.source, arguments.size, nullptr, 1, &kernelEvent));
        if (arguments.signalAfterCopy) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendSignalEvent(resources.copyList, resources.copyEvents[stage]));
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(resources.computeList, std::numeric_limits<uint64_t>::max()));
    if (resources.copyList != resources.computeList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(resources.copyList, std::numeric_limits<uint64_t>::max()));
    }

    return TestResult::Success;
}

static TestResult run(const KernelAndCopyPipelineImmediateArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{QueueProperties::create().disable()};
    Timer timer;

    auto queueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    if (queueDesc == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    PipelineResources resources{};
    queueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &queueDesc->desc, &resources.computeList));
    if (arguments.withCopyOffload) {
        queueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_COPY_OFFLOAD_HINT;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &queueDesc->desc, &resources.copyList));
    } else {
        resources.copyList = resources.computeList;
    }

    const size_t stages = arguments.stages;
    resources.kernelEvents.resize(stages);
    resources.copyEvents.resize(arguments.signalAfterCopy ? stages : 0);
    for (auto &event : resources.kernelEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &defaultIntelCounterBasedEventDesc, &event));
    }
    for (auto &event : resources.copyEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &defaultIntelCounterBasedEventDesc, &event));
    }

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &resources.source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &resources.destination));

    ze_module_handle_t module{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_eat_time.cl", "eat_time", &resources.kernel, &module, nullptr, ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(resources.kernel, 1, 1, 1));
    const int32_t operationsCount = static_cast<int32_t>(arguments.kernelTime);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(resources.kernel, 0, sizeof(operationsCount), &operationsCount));

    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        timer.measureStart();
        auto result = runSingleIteration(arguments, resources);
        if (result != TestResult::Success) {
            return result;
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(resources.kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, resources.source));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, resources.destination));
    for (auto event : resources.kernelEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    for (auto event : resources.copyEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    if (resources.copyList != resources.computeList) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(resources.copyList));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(resources.computeList));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelAndCopyPipelineImmediate> registerTestCase(run, Api::L0);
