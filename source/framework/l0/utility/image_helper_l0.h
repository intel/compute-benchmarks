/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/image_format_layout.h"
#include "framework/enum/image_format_type.h"
#include "framework/utility/error.h"
#include "framework/utility/image_helper.h"

#include <level_zero/ze_api.h>
#include <vector>

struct ImageHelperL0 : ImageHelper {
    struct Plane {
        ze_image_format_layout_t layout;
        uint32_t width;
        uint32_t height;
        size_t bytesPerPixel;
    };

    static inline ze_image_type_t getL0ImageTypeFromDimensions(const size_t *dimensions) {
        return getL0ImageType(ImageHelper::getImageTypeFromDimensions(dimensions));
    }

    static inline ze_image_type_t getL0ImageType(ImageType imageType) {
        switch (imageType) {
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

    static inline ze_image_format_layout_t getL0FormatLayout(ImageFormatLayout layout) {
        switch (layout) {
        case ImageFormatLayout::Layout8:
            return ZE_IMAGE_FORMAT_LAYOUT_8;
        case ImageFormatLayout::Layout8_8_8_8:
            return ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8;
        case ImageFormatLayout::Layout16_16_16_16:
            return ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16;
        case ImageFormatLayout::Layout32_32_32_32:
            return ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32;
        case ImageFormatLayout::Layout10_10_10_2:
            return ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2;
        case ImageFormatLayout::NV12:
            return ZE_IMAGE_FORMAT_LAYOUT_NV12;
        case ImageFormatLayout::P010:
            return ZE_IMAGE_FORMAT_LAYOUT_P010;
        default:
            FATAL_ERROR("Unknown image format layout");
        }
    }

    static inline ze_image_format_type_t getL0FormatType(ImageFormatType formatType) {
        switch (formatType) {
        case ImageFormatType::Uint:
            return ZE_IMAGE_FORMAT_TYPE_UINT;
        case ImageFormatType::Sint:
            return ZE_IMAGE_FORMAT_TYPE_SINT;
        case ImageFormatType::Unorm:
            return ZE_IMAGE_FORMAT_TYPE_UNORM;
        case ImageFormatType::Snorm:
            return ZE_IMAGE_FORMAT_TYPE_SNORM;
        case ImageFormatType::Float:
            return ZE_IMAGE_FORMAT_TYPE_FLOAT;
        default:
            FATAL_ERROR("Unknown image format type");
        }
    }

    static inline ze_image_format_t getImageFormat(ImageFormatLayout layout, ImageFormatType formatType) {
        return {getL0FormatLayout(layout), getL0FormatType(formatType),
                ZE_IMAGE_FORMAT_SWIZZLE_R, ZE_IMAGE_FORMAT_SWIZZLE_G, ZE_IMAGE_FORMAT_SWIZZLE_B, ZE_IMAGE_FORMAT_SWIZZLE_A};
    }

    static inline std::vector<Plane> getPlanes(ImageFormatLayout layout, uint32_t width, uint32_t height) {
        switch (layout) {
        case ImageFormatLayout::Layout8:
            return {{ZE_IMAGE_FORMAT_LAYOUT_8, width, height, getBytesPerPixel(layout)}};
        case ImageFormatLayout::Layout8_8_8_8:
            return {{ZE_IMAGE_FORMAT_LAYOUT_8_8_8_8, width, height, getBytesPerPixel(layout)}};
        case ImageFormatLayout::Layout16_16_16_16:
            return {{ZE_IMAGE_FORMAT_LAYOUT_16_16_16_16, width, height, getBytesPerPixel(layout)}};
        case ImageFormatLayout::Layout32_32_32_32:
            return {{ZE_IMAGE_FORMAT_LAYOUT_32_32_32_32, width, height, getBytesPerPixel(layout)}};
        case ImageFormatLayout::Layout10_10_10_2:
            return {{ZE_IMAGE_FORMAT_LAYOUT_10_10_10_2, width, height, getBytesPerPixel(layout)}};
        case ImageFormatLayout::NV12:
            return {{ZE_IMAGE_FORMAT_LAYOUT_8, width, height, 1},
                    {ZE_IMAGE_FORMAT_LAYOUT_8_8, width / 2, height / 2, 2}};
        case ImageFormatLayout::P010:
            return {{ZE_IMAGE_FORMAT_LAYOUT_16, width, height, 2},
                    {ZE_IMAGE_FORMAT_LAYOUT_16_16, width / 2, height / 2, 4}};
        default:
            FATAL_ERROR("Unknown image format layout");
        }
    }

    static inline void setImageDescSize(ze_image_desc_t &imageDesc, ImageType imageType, const size_t *size) {
        imageDesc.width = static_cast<uint32_t>(size[0]);
        imageDesc.height = 1u;
        imageDesc.depth = 1u;
        imageDesc.arraylevels = 0u;
        switch (imageType) {
        case ImageType::Image1D_Array:
            imageDesc.arraylevels = static_cast<uint32_t>(size[1]);
            break;
        case ImageType::Image2D_Array:
            imageDesc.height = static_cast<uint32_t>(size[1]);
            imageDesc.arraylevels = static_cast<uint32_t>(size[2]);
            break;
        default:
            imageDesc.height = static_cast<uint32_t>(size[1]);
            imageDesc.depth = static_cast<uint32_t>(size[2]);
            break;
        }
    }

    static inline bool validateImageDimensions(ze_device_handle_t device, const size_t *dimensions) {
        return validateImageDimensions(device, getImageTypeFromDimensions(dimensions), dimensions);
    }

    static inline bool validateImageDimensions(ze_device_handle_t device, ImageType imageType, const size_t *dimensions) {
        ze_device_image_properties_t deviceImageProperties{ZE_STRUCTURE_TYPE_DEVICE_IMAGE_PROPERTIES};
        if (zeDeviceGetImageProperties(device, &deviceImageProperties) != ZE_RESULT_SUCCESS) {
            return false;
        }

        switch (imageType) {
        case ImageType::Image1D:
        case ImageType::Image_Buffer:
            return dimensions[0] <= deviceImageProperties.maxImageDims1D;

        case ImageType::Image1D_Array:
            return dimensions[0] <= deviceImageProperties.maxImageDims1D &&
                   dimensions[1] <= deviceImageProperties.maxImageArraySlices;

        case ImageType::Image2D:
            return dimensions[0] <= deviceImageProperties.maxImageDims2D &&
                   dimensions[1] <= deviceImageProperties.maxImageDims2D;

        case ImageType::Image2D_Array:
            return dimensions[0] <= deviceImageProperties.maxImageDims2D &&
                   dimensions[1] <= deviceImageProperties.maxImageDims2D &&
                   dimensions[2] <= deviceImageProperties.maxImageArraySlices;

        case ImageType::Image3D:
            return dimensions[0] <= deviceImageProperties.maxImageDims3D &&
                   dimensions[1] <= deviceImageProperties.maxImageDims3D &&
                   dimensions[2] <= deviceImageProperties.maxImageDims3D;

        default:
            FATAL_ERROR("Unknown image type");
        }
    }
};
