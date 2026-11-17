/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/kernel_duration.h"

using namespace L0;

static TestResult run(const KernelDurationArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Gpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;

    constexpr uint64_t valuesPerWorkItem = 16;
    const ze_device_properties_t deviceProperties = levelzero.getDeviceProperties();
    const uint32_t groupSize = levelzero.getDeviceComputeProperties().maxGroupSizeX;
    const uint32_t groupCount = deviceProperties.numSlices * deviceProperties.numSubslicesPerSlice * deviceProperties.numEUsPerSubslice;
    const uint64_t workItemCount = static_cast<uint64_t>(groupSize) * groupCount;
    const uint64_t inputSize = workItemCount * valuesPerWorkItem * sizeof(float);

    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *input = nullptr;
    void *output = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, inputSize, 0, levelzero.device, &input));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, workItemCount * sizeof(float), 0, levelzero.device, &output));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, input, inputSize, BufferContents::Random, false));

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_sum_sixteen_values.cl", "sumSixteenValues", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSize, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(input), &input));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(output), &output));

    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_HOST_VISIBLE | ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP, 1};
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    const ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, 0};
    ze_event_handle_t event;
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));

    ze_command_list_handle_t cmdList;
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    const ze_group_count_t dispatchTraits{groupCount, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    ze_kernel_timestamp_result_t kernelTimestamp;
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &kernelTimestamp));
        const std::chrono::nanoseconds kernelTime = levelzero.getAbsoluteKernelExecutionTime(kernelTimestamp.global);
        statistics.pushValue(kernelTime, typeSelector.getUnit(), typeSelector.getType());
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, input));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, output));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelDuration> registerTestCase(run, Api::L0);
