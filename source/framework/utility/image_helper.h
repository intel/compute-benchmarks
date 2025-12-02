/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/image_type.h"

#include <cstddef>

struct ImageHelper {
    enum class ChannelOrder {
        Unknown,
        R,
        RG,
        RGBA,
    };

    enum class ChannelFormat {
        Unknown,
        Float,
    };

    static ImageType getImageTypeFromDimensions(const size_t *dimensions);
    static size_t getChannelCount(ChannelOrder order);
    static size_t getChannelFormatSize(ChannelFormat format);
    static size_t getImageSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions);
    static size_t getImageSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions, size_t alignment);
    static size_t getRowSizeInBytes(ChannelOrder order, ChannelFormat format, const size_t *dimensions, size_t alignment);
};
