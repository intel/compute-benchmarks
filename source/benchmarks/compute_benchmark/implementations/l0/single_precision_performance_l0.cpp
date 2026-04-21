/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/single_precision_performance.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>
#include <limits>

static TestResult run(const SinglePrecisionPerformanceArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigaFLOPS, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{};

    ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetProperties(levelzero.device, &deviceProperties));
    const size_t euCount = static_cast<size_t>(deviceProperties.numSlices) *
                           deviceProperties.numSubslicesPerSlice *
                           deviceProperties.numEUsPerSubslice;

    ze_device_compute_properties_t computeProperties{ZE_STRUCTURE_TYPE_DEVICE_COMPUTE_PROPERTIES};
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetComputeProperties(levelzero.device, &computeProperties));
    const size_t workgroupSize = computeProperties.maxTotalGroupSize;

    const size_t threadsPerEu = 8u;
    const size_t wavesCount = 4u;
    const size_t workgroupCount = euCount * threadsPerEu * wavesCount;
    const size_t gws = workgroupSize * workgroupCount;

    const size_t loopIterations = 200u;
    const size_t opsPerLoop = 128u;
    const size_t flopsPerOp = 2u;
    const uint64_t totalFlops = static_cast<uint64_t>(gws) * loopIterations * opsPerLoop * flopsPerOp;

    float multiplier = 1.0f;
    float addend = 1.0f;
    const float initialValue = 0.0f;
    const float expectedValue = static_cast<float>(loopIterations * opsPerLoop);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "compute_benchmark_single_precision_performance.cl", "single_precision_performance", &kernel, &module, nullptr); result != TestResult::Success) {
        return result;
    }
    void *buffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, gws * sizeof(float), 0, levelzero.device, &buffer));

    uint32_t loopIterationsUint = static_cast<uint32_t>(loopIterations);
    void *kernelArgs[] = {&buffer, &multiplier, &addend, &loopIterationsUint};

    const ze_group_count_t groupCount{static_cast<uint32_t>(workgroupCount), 1u, 1u};
    const ze_group_size_t groupSize{static_cast<uint32_t>(workgroupSize), 1u, 1u};

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdList));

    ze_event_handle_t profilingEvent{};
    if (arguments.useEvents) {
        auto eventDesc = defaultIntelCounterBasedEventDesc;
        eventDesc.flags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &profilingEvent));
    }

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, buffer, &initialValue, sizeof(float), gws * sizeof(float), nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

        if (arguments.useEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, groupCount, groupSize, kernelArgs, nullptr, profilingEvent, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(profilingEvent, std::numeric_limits<uint64_t>::max()));
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvent, &timestampResult));
            const auto commandTime = levelzero.getAbsoluteKernelExecutionTime(timestampResult.global);
            statistics.pushValue(commandTime, totalFlops, typeSelector.getUnit(), typeSelector.getType());
        } else {
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, groupCount, groupSize, kernelArgs, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            statistics.pushValue(timer.get(), totalFlops, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    auto testResult = TestResult::Success;
    std::vector<float> result(gws);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, result.data(), buffer, gws * sizeof(float), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

    for (size_t i = 0; i < gws; i++) {
        if (result[i] != expectedValue) {
            testResult = TestResult::VerificationFail;
            break;
        }
    }

    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(profilingEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return testResult;
}

static RegisterTestCaseImplementation<SinglePrecisionPerformance> registerTestCase(run, Api::L0);
