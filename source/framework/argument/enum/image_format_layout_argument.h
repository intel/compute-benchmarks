/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/image_format_layout.h"

struct ImageFormatLayoutArgument : EnumArgument<ImageFormatLayoutArgument, ImageFormatLayout> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "image format layout";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[] = {EnumType::Layout8, EnumType::Layout8_8_8_8, EnumType::Layout16_16_16_16, EnumType::Layout32_32_32_32, EnumType::Layout10_10_10_2, EnumType::NV12, EnumType::P010};
    static constexpr const char *enumValuesNames[] = {"8", "8_8_8_8", "16_16_16_16", "32_32_32_32", "10_10_10_2", "NV12", "P010"};
};
