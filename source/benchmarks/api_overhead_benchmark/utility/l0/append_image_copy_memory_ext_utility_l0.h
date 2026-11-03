/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/enum/usm_memory_placement.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/image_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include <level_zero/ze_api.h>
#include <vector>

template <typename Arguments, typename AppendImageCopy>
TestResult measureAppendImageCopyMemoryExt(const Arguments &arguments, const UsmMemoryPlacement bufferPlacement, Statistics &statistics,
                                           const AppendImageCopy &appendImageCopy) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    L0::QueueProperties queueProperties = L0::QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    L0::LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!levelzero.isExtensionSupported(ZE_IMAGE_COPY_EXT_NAME) || !ImageHelperL0::validateImageDimensions(levelzero.device, arguments.size)) {
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
                                    0u,
                                    ImageHelperL0::getL0ImageTypeFromDimensions(arguments.size),
                                    ImageHelperL0::getImageFormat(channelOrder, channelFormat),
                                    width,
                                    height,
                                    depth,
                                    1u,
                                    0u};
    ze_image_handle_t image{};
    ASSERT_ZE_RESULT_SUCCESS(zeImageCreate(levelzero.context, levelzero.device, &imageDesc, &image));

    const size_t imageSizeInBytes = ImageHelperL0::getImageSizeInBytes(channelOrder, channelFormat, arguments.size);
    void *buffer{};
    ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::allocate(bufferPlacement, levelzero, imageSizeInBytes, &buffer));

    const ze_image_region_t pixelRegion{0u, 0u, 0u, 1u, 1u, 1u};
    const ze_image_region_t *const imageRegion = arguments.wholeImage ? nullptr : &pixelRegion;
    const uint32_t bufferRowPitch = 0u;
    const uint32_t bufferSlicePitch = 0u;

    const uint32_t appendCount = static_cast<uint32_t>(arguments.appendCount);
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, appendCount};
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(appendCount);
    if (arguments.useEvent) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        for (auto j = 0u; j < appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    for (auto i = 0u; i < arguments.iterations; ++i) {
        timer.measureStart();
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(appendImageCopy(cmdList, image, buffer, imageRegion, bufferRowPitch, bufferSlicePitch, events[j]));
        }
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        statistics.pushValue(timer.get() / appendCount, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvent) {
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(L0::UsmHelper::deallocate(bufferPlacement, levelzero, buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(image));
    return TestResult::Success;
}
