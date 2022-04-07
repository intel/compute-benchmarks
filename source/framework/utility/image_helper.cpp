/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "image_helper.h"

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

size_t ImageHelper::getImageSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions) {
    return getChannelCount(order) * getChannelFormatSize(format) * dimensions[0] * dimensions[1] * dimensions[2];
}
