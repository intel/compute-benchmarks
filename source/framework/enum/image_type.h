/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

enum class ImageType {
    Unknown,

    Image1D,
    Image2D,
    Image3D,

    Image1D_Array,
    Image2D_Array,

    Image_Buffer,
};
