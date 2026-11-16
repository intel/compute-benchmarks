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

#include "definitions/sobel.h"
#include "utility/workload_input.h"

#include <limits>

using namespace L0;

static TestResult run(const SobelArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{};

    const size_t pixelCount = static_cast<size_t>(Sobel::width) * Sobel::height;
    const size_t imageSize = pixelCount * sizeof(uint32_t);
    const std::vector<uint32_t> inputImage = WorkloadInput::generateGrayscaleImage(Sobel::width, Sobel::height);
    std::vector<uint32_t> outputImage(pixelCount);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "algorithm_benchmark_sobel.cl", "sobel", &kernel, &module, nullptr); result != TestResult::Success) {
        return result;
    }

    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *inputBuffer{};
    void *outputBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, imageSize, 0, levelzero.device, &inputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, imageSize, 0, levelzero.device, &outputBuffer));

    const int32_t width = static_cast<int32_t>(Sobel::width);
    const int32_t height = static_cast<int32_t>(Sobel::height);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, Sobel::groupSize, Sobel::groupSize, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(inputBuffer), &inputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(outputBuffer), &outputBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(width), &width));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, sizeof(height), &height));

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, inputBuffer, inputImage.data(), imageSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr));
    const ze_group_count_t groupCount{Sobel::width / Sobel::groupSize, Sobel::height / Sobel::groupSize, 1u};
    for (auto launch = 0u; launch < Sobel::kernelLaunchCount; launch++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, outputImage.data(), outputBuffer, imageSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
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

static RegisterTestCaseImplementation<Sobel> registerTestCase(run, Api::L0);
