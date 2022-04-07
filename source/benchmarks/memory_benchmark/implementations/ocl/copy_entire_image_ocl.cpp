/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/image_helper_ocl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_entire_image.h"

#include <gtest/gtest.h>

static TestResult run(const CopyEntireImageArguments &arguments, Statistics &statistics) {
    // Setup
    cl_int retVal{};
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents).setForceBlitter(arguments.forceBlitter).allowCreationFail();
    Opencl opencl(queueProperties);
    if (opencl.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (!QueueFamiliesHelper::validateCapability(opencl.commandQueue, CL_QUEUE_CAPABILITY_TRANSFER_IMAGE_INTEL)) {
        return TestResult::DeviceNotCapable;
    }
    if (!ImageHelperOcl::validateImageDimensions(opencl.device, arguments.size)) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    const auto channelOrder = ImageHelperOcl::ChannelOrder::RGBA;
    const auto channelFormat = ImageHelperOcl::ChannelFormat::Float;

    // Create image
    const cl_image_format imageFormat = ImageHelperOcl::getImageFormat(channelOrder, channelFormat);
    cl_image_desc imageDescription = {};
    imageDescription.image_type = ImageHelperOcl::getOclImageTypeFromDimensions(arguments.size);
    imageDescription.image_width = arguments.size[0];
    imageDescription.image_height = arguments.size[1];
    imageDescription.image_depth = arguments.size[2];
    imageDescription.image_array_size = 1u;
    imageDescription.image_row_pitch = 0u;
    imageDescription.image_slice_pitch = 0u;
    imageDescription.num_mip_levels = 0u;
    imageDescription.num_samples = 0u;
    cl_mem srcImage = clCreateImage(opencl.context, 0, &imageFormat, &imageDescription, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem dstImage = clCreateImage(opencl.context, 0, &imageFormat, &imageDescription, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    const auto imageSizeInBytes = ImageHelperOcl::getImageSizeInBytes(channelOrder, channelFormat, arguments.size);

    // Warmup
    const size_t origin[] = {0, 0, 0};
    const size_t *region = arguments.size;
    ASSERT_CL_SUCCESS(clEnqueueCopyImage(opencl.commandQueue, srcImage, dstImage, origin, origin, region, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        cl_event profilingEvent{};
        cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueCopyImage(opencl.commandQueue, srcImage, dstImage, origin, origin, region, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), imageSizeInBytes, MeasurementUnit::GigabytesPerSecond, MeasurementType::Gpu);
        } else {
            statistics.pushValue(timer.get(), imageSizeInBytes, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
        }
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(dstImage));
    ASSERT_CL_SUCCESS(clReleaseMemObject(srcImage));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopyEntireImage> registerTestCase(run, Api::OpenCL);
