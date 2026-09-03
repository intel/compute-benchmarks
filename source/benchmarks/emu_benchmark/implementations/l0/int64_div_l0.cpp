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

#include "definitions/int64_div.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>
#include <limits>

using namespace L0;

static TestResult run(const Int64DivArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{};
    if (arguments.useEvents && !levelzero.isCounterBasedEventsSupported()) {
        return TestResult::ApiNotCapable;
    }

    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const int64_t initialValue = std::numeric_limits<int64_t>::max() - 1;
    int64_t divisor = 2;
    uint32_t loopIterations = 640u;

    if (arguments.workgroupSize > levelzero.getDeviceComputeProperties().maxGroupSizeX) {
        return TestResult::DeviceNotCapable;
    }

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "emu_benchmark_int64_div.cl", "int64_div", &kernel, &module, nullptr); result != TestResult::Success) {
        return result;
    }

    void *buffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, gws * sizeof(int64_t), 0, levelzero.device, &buffer));

    void *kernelArgs[] = {&buffer, &divisor, &loopIterations};
    const ze_group_count_t groupCount{static_cast<uint32_t>(arguments.workgroupCount), 1u, 1u};
    const ze_group_size_t groupSize{static_cast<uint32_t>(arguments.workgroupSize), 1u, 1u};

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdList));

    ze_event_handle_t profilingEvent{};
    if (arguments.useEvents) {
        auto eventDesc = defaultIntelCounterBasedEventDesc;
        eventDesc.flags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &profilingEvent));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmdList, buffer, &initialValue, sizeof(initialValue), gws * sizeof(int64_t), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.useEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, groupCount, groupSize, kernelArgs, nullptr, profilingEvent, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(profilingEvent, std::numeric_limits<uint64_t>::max()));
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvent, &timestampResult));
            statistics.pushValue(levelzero.getAbsoluteKernelExecutionTime(timestampResult.global), typeSelector.getUnit(), typeSelector.getType());
        } else {
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, groupCount, groupSize, kernelArgs, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    int64_t result{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, &result, buffer, sizeof(result), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    const auto testResult = (result == initialValue) ? TestResult::Success : TestResult::VerificationFail;

    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(profilingEvent));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return testResult;
}

static RegisterTestCaseImplementation<Int64Div> registerTestCase(run, Api::L0);
