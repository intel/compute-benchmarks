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

#include "definitions/mandelbrot.h"

#include <limits>

using namespace L0;

static TestResult run(const MandelbrotArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero{};

    const size_t pixelCount = static_cast<size_t>(Mandelbrot::width) * Mandelbrot::height;
    const size_t imageSize = pixelCount * sizeof(float);
    std::vector<float> image(pixelCount);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "algorithm_benchmark_mandelbrot.cl", "mandelbrot", &kernel, &module, nullptr); result != TestResult::Success) {
        return result;
    }

    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *imageBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, imageSize, 0, levelzero.device, &imageBuffer));

    const int32_t width = static_cast<int32_t>(Mandelbrot::width);
    const int32_t height = static_cast<int32_t>(Mandelbrot::height);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, Mandelbrot::groupSize, Mandelbrot::groupSize, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(imageBuffer), &imageBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(width), &width));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(height), &height));

    const ze_group_count_t groupCount{Mandelbrot::width / Mandelbrot::groupSize, Mandelbrot::height / Mandelbrot::groupSize, 1u};
    ze_command_queue_desc_t cmdListDesc = levelzero.commandQueueDesc;
    cmdListDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    cmdListDesc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto launch = 0u; launch < Mandelbrot::kernelLaunchCount; launch++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, image.data(), imageBuffer, imageSize, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, imageBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<Mandelbrot> registerTestCase(run, Api::L0);
