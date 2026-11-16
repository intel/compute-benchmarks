/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_memory_placement.h"
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/image_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/buffer_contents_helper.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include <cstring>
#include <limits>
#include <vector>

namespace L0 {

enum class ImageCopyDirection {
    FromMemory,
    ToMemory,
};

struct ImageCopyPlane {
    ze_image_handle_t image;
    ze_image_region_t region;
    size_t memoryOffset;
    size_t sizeInBytes;
    ze_event_handle_t event;
};

template <typename Arguments>
TestResult measureImageCopyWithMemory(const Arguments &arguments, UsmMemoryPlacement memoryPlacement, ImageCopyDirection direction, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    const ImageType imageType = arguments.imageType;
    const size_t *size = arguments.size;
    const bool isPlanar = ImageHelperL0::isPlanar(arguments.layout);
    if (!ImageHelperL0::isSizeValidForImageType(imageType, size)) {
        return TestResult::InvalidArgs;
    }
    if (isPlanar && (imageType != ImageType::Image2D || size[0] % 2 != 0 || size[1] % 2 != 0)) {
        return TestResult::InvalidArgs;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!levelzero.isCounterBasedEventsSupported()) {
        return TestResult::DeviceNotCapable;
    }
    if (!ImageHelperL0::validateImageDimensions(levelzero.device, imageType, size)) {
        return TestResult::DeviceNotCapable;
    }
    if (isPlanar && !levelzero.isExtensionSupported(ZE_IMAGE_VIEW_PLANAR_EXT_NAME)) {
        return TestResult::DeviceNotCapable;
    }

    ze_image_desc_t imageDesc{ZE_STRUCTURE_TYPE_IMAGE_DESC};
    imageDesc.type = ImageHelperL0::getL0ImageType(imageType);
    imageDesc.format = ImageHelperL0::getImageFormat(arguments.layout, arguments.formatType);
    ImageHelperL0::setImageDescSize(imageDesc, imageType, size);
    ze_image_handle_t image{};
    const ze_result_t imageCreateResult = zeImageCreate(levelzero.context, levelzero.device, &imageDesc, &image);
    if (imageCreateResult == ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT) {
        return TestResult::DeviceNotCapable;
    }
    ASSERT_ZE_RESULT_SUCCESS(imageCreateResult);

    ze_event_counter_based_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_COUNTER_BASED_DESC};
    eventDesc.flags = ZE_EVENT_COUNTER_BASED_FLAG_IMMEDIATE | ZE_EVENT_COUNTER_BASED_FLAG_HOST_VISIBLE;
    if (arguments.useEvents) {
        eventDesc.flags |= ZE_EVENT_COUNTER_BASED_FLAG_DEVICE_TIMESTAMP;
    }
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_HOST;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    const auto planeFormats = ImageHelperL0::getPlanes(arguments.layout, static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]));
    std::vector<ImageCopyPlane> planes{};
    size_t totalSizeInBytes = 0;
    for (auto planeIndex = 0u; planeIndex < planeFormats.size(); planeIndex++) {
        const auto &planeFormat = planeFormats[planeIndex];
        ImageCopyPlane plane{};
        plane.image = image;
        plane.region = {0u, 0u, 0u, static_cast<uint32_t>(size[0]), static_cast<uint32_t>(size[1]), static_cast<uint32_t>(size[2])};
        if (isPlanar) {
            ze_image_view_planar_ext_desc_t planeViewDesc{ZE_STRUCTURE_TYPE_IMAGE_VIEW_PLANAR_EXT_DESC};
            planeViewDesc.planeIndex = planeIndex;
            ze_image_desc_t viewDesc = imageDesc;
            viewDesc.pNext = &planeViewDesc;
            viewDesc.format.layout = planeFormat.layout;
            viewDesc.width = planeFormat.width;
            viewDesc.height = planeFormat.height;
            ASSERT_ZE_RESULT_SUCCESS(zeImageViewCreateExt(levelzero.context, levelzero.device, &viewDesc, image, &plane.image));
            plane.region = {0u, 0u, 0u, planeFormat.width, planeFormat.height, 1u};
        }
        plane.memoryOffset = totalSizeInBytes;
        plane.sizeInBytes = static_cast<size_t>(plane.region.width) * plane.region.height * plane.region.depth * planeFormat.bytesPerPixel;
        totalSizeInBytes += plane.sizeInBytes;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCounterBasedCreate(levelzero.context, levelzero.device, &eventDesc, &plane.event));
        planes.push_back(plane);
    }

    ze_command_list_handle_t cmdList{};
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, queueProperties.selectedEngine);
    commandQueueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    if (!arguments.forceBlitter) {
        commandQueueDesc->desc.flags |= ZE_COMMAND_QUEUE_FLAG_COPY_OFFLOAD_HINT;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdList));

    void *memory{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(memoryPlacement, levelzero, totalSizeInBytes, &memory));
    std::vector<uint8_t> initialData(totalSizeInBytes);
    BufferContentsHelper::fill(initialData.data(), totalSizeInBytes, BufferContents::Random);
    if (isDeviceMemoryType(memoryPlacement)) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, memory, initialData.data(), totalSizeInBytes, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    } else {
        std::memcpy(memory, initialData.data(), totalSizeInBytes);
    }

    const auto appendImageCopy = [&](ImageCopyDirection copyDirection, const ImageCopyPlane &plane) {
        void *planeMemory = static_cast<uint8_t *>(memory) + plane.memoryOffset;
        if (copyDirection == ImageCopyDirection::FromMemory) {
            return zeCommandListAppendImageCopyFromMemory(cmdList, plane.image, planeMemory, &plane.region, plane.event, 0, nullptr);
        }
        return zeCommandListAppendImageCopyToMemory(cmdList, planeMemory, plane.image, &plane.region, plane.event, 0, nullptr);
    };

    if (direction == ImageCopyDirection::ToMemory) {
        for (const auto &plane : planes) {
            ASSERT_ZE_RESULT_SUCCESS(appendImageCopy(ImageCopyDirection::FromMemory, plane));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(planes.back().event, std::numeric_limits<uint64_t>::max()));
    }

    Timer timer;
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (const auto &plane : planes) {
            ASSERT_ZE_RESULT_SUCCESS(appendImageCopy(direction, plane));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(planes.back().event, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t firstTimestamp{};
            ze_kernel_timestamp_result_t lastTimestamp{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(planes.front().event, &firstTimestamp));
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(planes.back().event, &lastTimestamp));
            const ze_kernel_timestamp_data_t copyTimestamp{firstTimestamp.global.kernelStart, lastTimestamp.global.kernelEnd};
            statistics.pushValue(levelzero.getAbsoluteKernelExecutionTime(copyTimestamp), totalSizeInBytes, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), totalSizeInBytes, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(memoryPlacement, levelzero, memory));
    for (const auto &plane : planes) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(plane.event));
        if (plane.image != image) {
            ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(plane.image));
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(image));
    return TestResult::Success;
}

} // namespace L0
