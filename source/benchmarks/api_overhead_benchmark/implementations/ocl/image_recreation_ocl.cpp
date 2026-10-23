/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/buffer_contents_helper_ocl.h"
#include "framework/ocl/utility/image_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/image_recreation.h"

#include <gtest/gtest.h>

using namespace OCL;

static TestResult run(const ImageRecreationArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Opencl opencl;
    if (!ImageHelperOcl::validateImageDimensions(opencl.device, arguments.size)) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;
    cl_int retVal{};

    const auto channelOrder = ImageHelperOcl::ChannelOrder::RGBA;
    const auto channelFormat = ImageHelperOcl::ChannelFormat::Float;
    const cl_image_format imageFormat = ImageHelperOcl::getImageFormat(channelOrder, channelFormat);
    const cl_image_desc imageDescription{ImageHelperOcl::getOclImageTypeFromDimensions(arguments.size),
                                         arguments.size[0],
                                         arguments.size[1],
                                         arguments.size[2],
                                         1u};

    const auto imageSizeInBytes = ImageHelperOcl::getImageSizeInBytes(channelOrder, channelFormat, arguments.size);
    const cl_mem sourceBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, imageSizeInBytes, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(BufferContentsHelperOcl::fillBuffer(opencl.commandQueue, sourceBuffer, imageSizeInBytes, BufferContents::Random));

    const size_t origin[] = {0, 0, 0};
    const size_t singlePixelRegion[] = {1, 1, 1};
    const size_t *region = arguments.writeWholeImage ? static_cast<const size_t *>(arguments.size) : singlePixelRegion;

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        const cl_mem image = clCreateImage(opencl.context, CL_MEM_READ_WRITE, &imageFormat, &imageDescription, nullptr, &retVal);
        ASSERT_CL_SUCCESS(retVal);
        ASSERT_CL_SUCCESS(clEnqueueCopyBufferToImage(opencl.commandQueue, sourceBuffer, image, 0, origin, region, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        ASSERT_CL_SUCCESS(clReleaseMemObject(image));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_CL_SUCCESS(clReleaseMemObject(sourceBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ImageRecreation> registerTestCase(run, Api::OpenCL);
