/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "image_helper.h"

#include "framework/enum/image_format_layout.h"
#include "framework/utility/alignment.h"
#include "framework/utility/error.h"

ImageType ImageHelper::getImageTypeFromDimensions(const size_t *dimensions) {
    if (dimensions[2] > 1) {
        return ImageType::Image3D;
    }
    if (dimensions[1] > 1) {
        return ImageType::Image2D;
    }
    return ImageType::Image1D;
}

size_t ImageHelper::getChannelCount(ChannelOrder order) {
    switch (order) {
    case ChannelOrder::R:
        return 1;
    case ChannelOrder::RG:
        return 2;
    case ChannelOrder::RGBA:
        return 4;
    default:
        FATAL_ERROR("Unknown channel order");
    }
}

size_t ImageHelper::getChannelFormatSize(ChannelFormat format) {
    switch (format) {
    case ChannelFormat::Float:
        return 4;
    default:
        FATAL_ERROR("Unknown channel format");
    }
}

size_t ImageHelper::getRowSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions, size_t alignment) {
    return alignUp(getChannelCount(order) * getChannelFormatSize(format) * dimensions[0], alignment);
}

size_t ImageHelper::getImageSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions, size_t alignment) {
    return getRowSizeInBytes(order, format, dimensions, alignment) * dimensions[1] * dimensions[2];
}

size_t ImageHelper::getImageSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions) {
    return getRowSizeInBytes(order, format, dimensions, 1u) * dimensions[1] * dimensions[2];
}

bool ImageHelper::isPlanar(ImageFormatLayout layout) {
    return layout == ImageFormatLayout::NV12 || layout == ImageFormatLayout::P010;
}

size_t ImageHelper::getBytesPerPixel(ImageFormatLayout layout) {
    switch (layout) {
    case ImageFormatLayout::Layout8:
        return 1;
    case ImageFormatLayout::Layout8_8_8_8:
    case ImageFormatLayout::Layout10_10_10_2:
        return 4;
    case ImageFormatLayout::Layout16_16_16_16:
        return 8;
    case ImageFormatLayout::Layout32_32_32_32:
        return 16;
    default:
        FATAL_ERROR("Unknown single-planar image format layout");
    }
}

bool ImageHelper::isSizeValidForImageType(ImageType imageType, const size_t *size) {
    if (size[0] == 0 || size[1] == 0 || size[2] == 0) {
        return false;
    }
    switch (imageType) {
    case ImageType::Image1D:
        return size[1] == 1 && size[2] == 1;
    case ImageType::Image1D_Array:
    case ImageType::Image2D:
        return size[2] == 1;
    case ImageType::Image3D:
    case ImageType::Image2D_Array:
        return true;
    default:
        return false;
    }
}
