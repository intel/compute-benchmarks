/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/image_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/set_kernel_arg_image.h"
#include "utility/l0/kernel_set_argument_value_utility_l0.h"

using namespace L0;

static TestResult run(const KernelSetArgumentValueImageArguments &arguments, Statistics &statistics) {
    if (isNoopRun()) {
        statistics.pushUnitAndType(MeasurementUnit::Microseconds, MeasurementType::Cpu);
        return TestResult::Nooped;
    }
    if (arguments.argumentIndex >= kernelSetArgumentValueArgumentCount) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
    constexpr uint32_t imageWidth = 128u;
    constexpr uint32_t imageHeight = 128u;

    constexpr size_t imageSize[3] = {imageWidth, imageHeight, 1u};
    if (!ImageHelperL0::validateImageDimensions(levelzero.device, imageSize)) {
        return TestResult::DeviceNotCapable;
    }

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_image_arguments.cl", "kernelWith8Images", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }

    const ze_image_desc_t imageDesc{ZE_STRUCTURE_TYPE_IMAGE_DESC,
                                    nullptr,
                                    ZE_IMAGE_FLAG_KERNEL_WRITE,
                                    ZE_IMAGE_TYPE_2D,
                                    ImageHelperL0::getImageFormat(ImageHelperL0::ChannelOrder::R, ImageHelperL0::ChannelFormat::Float),
                                    imageWidth,
                                    imageHeight,
                                    1u,
                                    1u,
                                    0u};
    std::array<ze_image_handle_t, 2> images{};
    for (auto &image : images) {
        ASSERT_ZE_RESULT_SUCCESS(zeImageCreate(levelzero.context, levelzero.device, &imageDesc, &image));
    }

    const TestResult result = measureKernelSetArgumentValue(kernel, arguments.argumentIndex, images, arguments.differentValues, arguments.iterations, statistics);

    for (auto &image : images) {
        ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(image));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return result;
}

static RegisterTestCaseImplementation<KernelSetArgumentValueImage> registerTestCase(run, Api::L0);
