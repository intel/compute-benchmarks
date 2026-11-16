/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/image_type.h"

struct ImageTypeArgument : EnumArgument<ImageTypeArgument, ImageType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "image type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[] = {EnumType::Image1D, EnumType::Image2D, EnumType::Image3D, EnumType::Image1D_Array, EnumType::Image2D_Array};
    static constexpr const char *enumValuesNames[] = {"1D", "2D", "3D", "1DArray", "2DArray"};
};
