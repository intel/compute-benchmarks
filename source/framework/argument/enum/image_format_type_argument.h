/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/image_format_type.h"

struct ImageFormatTypeArgument : EnumArgument<ImageFormatTypeArgument, ImageFormatType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "image format type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[] = {EnumType::Uint, EnumType::Sint, EnumType::Unorm, EnumType::Snorm, EnumType::Float};
    static constexpr const char *enumValuesNames[] = {"Uint", "Sint", "Unorm", "Snorm", "Float"};
};
