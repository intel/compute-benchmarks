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

#include "definitions/simple_add.h"

#include <limits>

using namespace L0;

static TestResult run(const SimpleAddArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{};

    const size_t bufferSize = SimpleAdd::elementCount * sizeof(int32_t);
    const std::vector<int32_t> input(SimpleAdd::elementCount, 1);
    std::vector<int32_t> output(SimpleAdd::elementCount);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "algorithm_benchmark_simple_add.cl", "simple_add", &kernel, &module, nullptr); result != TestResult::Success) {
        return result;
    }

    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *inputBuffer{};
    void *outputBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &inputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &outputBuffer));

    const int32_t elementCount = static_cast<int32_t>(SimpleAdd::elementCount);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(elementCount), &elementCount));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(inputBuffer), &inputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(outputBuffer), &outputBuffer));

    const ze_group_count_t groupCount{1u, 1u, 1u};
    ze_command_queue_desc_t cmdListDesc = levelzero.commandQueueDesc;
    cmdListDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    cmdListDesc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, inputBuffer, input.data(), bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, output.data(), outputBuffer, bufferSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, outputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, inputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<SimpleAdd> registerTestCase(run, Api::L0);
