/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/utility/error.h"
#include "framework/utility/image_helper.h"

#include <level_zero/ze_api.h>

struct ImageHelperL0 : ImageHelper {
    static inline ze_image_type_t getL0ImageTypeFromDimensions(const size_t *dimensions) {
        switch (ImageHelper::getImageTypeFromDimensions(dimensions)) {
        case ImageType::Image1D:
            return ZE_IMAGE_TYPE_1D;
        case ImageType::Image2D:
            return ZE_IMAGE_TYPE_2D;
        case ImageType::Image3D:
            return ZE_IMAGE_TYPE_3D;
        case ImageType::Image1D_Array:
            return ZE_IMAGE_TYPE_1DARRAY;
        case ImageType::Image2D_Array:
            return ZE_IMAGE_TYPE_2DARRAY;
        case ImageType::Image_Buffer:
            return ZE_IMAGE_TYPE_BUFFER;
        default:
            FATAL_ERROR("Unknown image type");
        }
    }

    static inline ze_image_format_t getImageFormat(ChannelOrder order, ChannelFormat format) {
        ze_image_format_t imageFormat = {};
        switch (order) {
        case ChannelOrder::R:
            imageFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32;
            imageFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
            imageFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_0;
            imageFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
            imageFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
            break;
        case ChannelOrder::RG:
            imageFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32;
            imageFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
            imageFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
            imageFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_0;
            imageFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_1;
            break;
        case ChannelOrder::RGBA:
            imageFormat.layout = ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32;
            imageFormat.x = ZE_IMAGE_FORMAT_SWIZZLE_R;
            imageFormat.y = ZE_IMAGE_FORMAT_SWIZZLE_G;
            imageFormat.z = ZE_IMAGE_FORMAT_SWIZZLE_B;
            imageFormat.w = ZE_IMAGE_FORMAT_SWIZZLE_A;
            break;
        default:
            FATAL_ERROR("Unknown channel order");
        }

        switch (format) {
        case ChannelFormat::Float:
            imageFormat.type = ZE_IMAGE_FORMAT_TYPE_FLOAT;
            break;
        default:
            FATAL_ERROR("Unknown channel format");
        }

        return imageFormat;
    }

    static inline bool validateImageDimensions(ze_device_handle_t device, const size_t *dimensions) {
        ze_device_image_properties_t deviceImageProperties{ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES};
        if (zeDeviceGetImageProperties(device, &deviceImageProperties) != ZE_RESULT_SUCCESS) {
            return false;
        }

        switch (getImageTypeFromDimensions(dimensions)) {
        case ImageType::Image1D:
        case ImageType::Image1D_Array:
        case ImageType::Image_Buffer:
            return dimensions[0] <= deviceImageProperties.maxImageDims1D;

        case ImageType::Image2D:
        case ImageType::Image2D_Array:
            return dimensions[0] <= deviceImageProperties.maxImageDims2D &&
                   dimensions[1] <= deviceImageProperties.maxImageDims2D;

        case ImageType::Image3D:
            return dimensions[0] <= deviceImageProperties.maxImageDims3D &&
                   dimensions[1] <= deviceImageProperties.maxImageDims3D &&
                   dimensions[2] <= deviceImageProperties.maxImageDims3D;

        default:
            FATAL_ERROR("Unknown image type");
        }
    }
};
