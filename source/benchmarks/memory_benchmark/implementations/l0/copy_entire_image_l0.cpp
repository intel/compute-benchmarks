/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/image_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_entire_image.h"

#include <gtest/gtest.h>

static TestResult run(const CopyEntireImageArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!ImageHelperL0::validateImageDimensions(levelzero.device, arguments.size)) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    const uint64_t timerResolution = levelzero.getTimerResolution(levelzero.device);
    const auto channelOrder = ImageHelperL0::ChannelOrder::RGBA;
    const auto channelFormat = ImageHelperL0::ChannelFormat::Float;

    // Create image
    ze_image_desc_t imageDesc = {ZE_STRUCTURE_TYPE_IMAGE_DESC};
    imageDesc.type = ImageHelperL0::getL0ImageTypeFromDimensions(arguments.size);
    imageDesc.format = ImageHelperL0::getImageFormat(channelOrder, channelFormat);
    imageDesc.width = static_cast<uint32_t>(arguments.size[0]);
    imageDesc.height = static_cast<uint32_t>(arguments.size[1]);
    imageDesc.depth = static_cast<uint32_t>(arguments.size[2]);
    imageDesc.arraylevels = 1u;
    ze_image_handle_t srcImage = {};
    ASSERT_ZE_RESULT_SUCCESS(zeImageCreate(levelzero.context, levelzero.device, &imageDesc, &srcImage));
    ze_image_handle_t dstImage = {};
    ASSERT_ZE_RESULT_SUCCESS(zeImageCreate(levelzero.context, levelzero.device, &imageDesc, &dstImage));
    const auto imageSizeInBytes = ImageHelperL0::getImageSizeInBytes(channelOrder, channelFormat, arguments.size);

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useEvents) {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Make buffers resident
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeImageResident(levelzero.context, levelzero.device, srcImage));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeImageResident(levelzero.context, levelzero.device, dstImage));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendImageCopy(cmdList, dstImage, srcImage, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            auto commandTime = std::chrono::nanoseconds(timestampResult.global.kernelEnd - timestampResult.global.kernelStart);
            commandTime *= timerResolution;
            statistics.pushValue(commandTime, imageSizeInBytes, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), imageSizeInBytes, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Evict buffers
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictImage(levelzero.context, levelzero.device, srcImage));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictImage(levelzero.context, levelzero.device, dstImage));

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(srcImage));
    ASSERT_ZE_RESULT_SUCCESS(zeImageDestroy(dstImage));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopyEntireImage> registerTestCase(run, Api::L0);
