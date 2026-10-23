/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/image_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/image_recreation.h"

#include <gtest/gtest.h>
#include <limits>

using namespace L0;

static TestResult run(const ImageRecreationArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    if (!ImageHelperL0::validateImageDimensions(levelzero.device, arguments.size)) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    const auto channelOrder = ImageHelperL0::ChannelOrder::RGBA;
    const auto channelFormat = ImageHelperL0::ChannelFormat::Float;
    const auto width = static_cast<uint32_t>(arguments.size[0]);
    const auto height = static_cast<uint32_t>(arguments.size[1]);
    const auto depth = static_cast<uint32_t>(arguments.size[2]);
    const ze_image_desc_t imageDesc{ZE_STRUCTURE_TYPE_IMAGE_DESC,
                                    nullptr,
                                    ZE_IMAGE_FLAG_KERNEL_WRITE,
                                    ImageHelperL0::getL0ImageTypeFromDimensions(arguments.size),
                                    ImageHelperL0::getImageFormat(channelOrder, channelFormat),
                                    width,
                                    height,
                                    depth,
                                    1u,
                                    0u};

    const auto imageSizeInBytes = ImageHelperL0::getImageSizeInBytes(channelOrder, channelFormat, arguments.size);
    const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *sourceData = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, imageSizeInBytes, 0, levelzero.device, &sourceData));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, sourceData, imageSizeInBytes, BufferContents::Random, false));

    const ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC,
                                                   nullptr,
                                                   0u,
                                                   0u,
                                                   ZE_COMMAND_QUEUE_FLAG_IN_ORDER,
                                                   ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS,
                                                   ZE_COMMAND_QUEUE_PRIORITY_NORMAL};
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    const ze_image_region_t region = arguments.writeWholeImage ? ze_image_region_t{0u, 0u, 0u, width, height, depth}
                                                               : ze_image_region_t{0u, 0u, 0u, 1u, 1u, 1u};

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ze_image_handle_t image{};
        ASSERT_ZE_RESULT_SUCCESS(zeImageCreate(levelzero.context, levelzero.device, &imageDesc, &image));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendImageCopyFromMemory(cmdList, image, sourceData, &region, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(image));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, sourceData));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ImageRecreation> registerTestCase(run, Api::L0);
