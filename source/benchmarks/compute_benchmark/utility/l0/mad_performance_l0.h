/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/queue_families_helper.h"
#include "framework/test_case/test_case_argument_container.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include "../mad_performance.h"

#include <level_zero/ze_api.h>
#include <level_zero/zer_api.h>
#include <limits>
#include <vector>

inline TestResult checkMadDataTypeSupport(L0::LevelZero &levelzero, MadPerformance::DataType dataType) {
    ze_device_module_properties_t moduleProperties{ZE_STRUCTURE_TYPE_DEVICE_MODULE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetModuleProperties(levelzero.device, &moduleProperties));
    if (dataType == MadPerformance::DataType::Half && (moduleProperties.flags & ZE_DEVICE_MODULE_FLAG_FP16) == 0) {
        return TestResult::DeviceNotCapable;
    }
    if (dataType == MadPerformance::DataType::Double && (moduleProperties.flags & ZE_DEVICE_MODULE_FLAG_FP64) == 0) {
        return TestResult::DeviceNotCapable;
    }
    return TestResult::Success;
}

inline TestResult runMadPerformanceL0(const TestCaseArgumentContainer &arguments, Statistics &statistics, MadPerformance::DataType dataType,
                                      size_t vectorWidth, bool useEvents) {
    const MeasurementFields typeSelector(MadPerformance::getMeasurementUnit(dataType), useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0::LevelZero levelzero{};

    if (auto result = checkMadDataTypeSupport(levelzero, dataType); result != TestResult::Success) {
        return result;
    }

    auto commandQueueDesc = L0::QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    if (commandQueueDesc == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    commandQueueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;

    ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    const size_t euCount = static_cast<size_t>(deviceProperties.numSlices) *
                           deviceProperties.numSubslicesPerSlice *
                           deviceProperties.numEUsPerSubslice;

    ze_device_compute_properties_t computeProperties{ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetComputeProperties(levelzero.device, &computeProperties));
    const size_t workgroupSize = computeProperties.maxTotalGroupSize;
    const size_t workgroupCount = euCount * MadPerformance::threadsPerEu * MadPerformance::wavesCount;
    const size_t gws = workgroupSize * workgroupCount;
    const size_t bufferSize = gws * vectorWidth * MadPerformance::getScalarSize(dataType);
    const uint64_t totalOperations = MadPerformance::getOperationsPerKernel(gws);

    std::vector<uint8_t> multiplier = MadPerformance::getOne(dataType);
    std::vector<uint8_t> addend = MadPerformance::getOne(dataType);
    uint32_t loopIterations = MadPerformance::getLoopIterations(vectorWidth);
    const uint8_t initialValue = 0u;

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    const std::string buildOptions = MadPerformance::getBuildOptions(dataType, vectorWidth);
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "compute_benchmark_mad_performance.cl", "mad_performance", &kernel, &module, buildOptions.c_str()); result != TestResult::Success) {
        return result;
    }
    void *buffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, bufferSize, 0, levelzero.device, &buffer));

    void *kernelArgs[] = {&buffer, multiplier.data(), addend.data(), &loopIterations};

    const ze_group_count_t groupCount{static_cast<uint32_t>(workgroupCount), 1u, 1u};
    const ze_group_size_t groupSize{static_cast<uint32_t>(workgroupSize), 1u, 1u};

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdList));

    ze_event_handle_t profilingEvent{};
    if (useEvents) {
        auto eventDesc = defaultIntelCounterBasedEventDesc;
        eventDesc.flags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &profilingEvent));
    }

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, buffer, &initialValue, sizeof(initialValue), bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

        if (useEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, groupCount, groupSize, kernelArgs, nullptr, profilingEvent, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(profilingEvent, std::numeric_limits<uint64_t>::max()));
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvent, &timestampResult));
            const auto commandTime = levelzero.getAbsoluteKernelExecutionTime(timestampResult.global);
            statistics.pushValue(commandTime, totalOperations, typeSelector.getUnit(), typeSelector.getType());
        } else {
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, groupCount, groupSize, kernelArgs, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            statistics.pushValue(timer.get(), totalOperations, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    std::vector<uint8_t> result(bufferSize);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, result.data(), buffer, bufferSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    const auto testResult = MadPerformance::verify(dataType, vectorWidth, result) ? TestResult::Success : TestResult::VerificationFail;

    if (useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(profilingEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return testResult;
}
