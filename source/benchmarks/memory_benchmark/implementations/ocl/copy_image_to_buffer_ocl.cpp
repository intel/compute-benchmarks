/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/image_helper_ocl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/copy_image_to_buffer.h"

#include <gtest/gtest.h>

static TestResult run(const CopyImageToBufferArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

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
    if (!ImageHelperOcl::validateImageDimensions(opencl.device, arguments.region)) {
        return TestResult::DeviceNotCapable;
    }
    cl_event profilingEvent{};
    cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;
    Timer timer;
    const auto channelOrder = ImageHelperOcl::ChannelOrder::RGBA;
    const auto channelFormat = ImageHelperOcl::ChannelFormat::Float;

    // Create image and buffer
    const cl_image_format imageFormat = ImageHelperOcl::getImageFormat(channelOrder, channelFormat);
    cl_image_desc imageDescription = {};
    imageDescription.image_type = ImageHelperOcl::getOclImageTypeFromDimensions(arguments.region);
    imageDescription.image_width = arguments.region[0];
    imageDescription.image_height = arguments.region[1];
    imageDescription.image_depth = arguments.region[2];
    imageDescription.image_array_size = 1u;
    imageDescription.image_row_pitch = 0u;
    imageDescription.image_slice_pitch = 0u;
    imageDescription.num_mip_levels = 0u;
    imageDescription.num_samples = 0u;
    cl_mem srcImage = clCreateImage(opencl.context, 0, &imageFormat, &imageDescription, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem dstBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, arguments.size, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    const auto imageSizeInBytes = ImageHelperOcl::getImageSizeInBytes(channelOrder, channelFormat, arguments.region);

    // Warmup
    const size_t origin[] = {0, 0, 0};
    const size_t region[] = {arguments.region[0], arguments.region[1], arguments.region[2]};

    ASSERT_CL_SUCCESS(clEnqueueCopyImageToBuffer(opencl.commandQueue, srcImage, dstBuffer, origin, region, 0, 0, nullptr, eventForEnqueue));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    if (eventForEnqueue) {
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueCopyImageToBuffer(opencl.commandQueue, srcImage, dstBuffer, origin, region, 0, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), imageSizeInBytes, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), imageSizeInBytes, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(srcImage));
    ASSERT_CL_SUCCESS(clReleaseMemObject(dstBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<CopyImageToBuffer> registerTestCase(run, Api::OpenCL);
