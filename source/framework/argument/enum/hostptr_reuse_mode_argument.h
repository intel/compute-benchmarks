/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/hostptr_reuse_mode.h"

struct HostptrBufferReuseModeArgument : EnumArgument<HostptrBufferReuseModeArgument, HostptrReuseMode> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "Buffer hostptr reuse mode";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[4] = {EnumType::Aligned4KB, EnumType::Misaligned, EnumType::Usm, EnumType::Map};
    const static inline std::string enumValuesNames[4] = {"Aligned4KB", "Misaligned", "Usm", "Map"};
};

struct HostptrImageReuseModeArgument : EnumArgument<HostptrImageReuseModeArgument, HostptrReuseMode> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "Image hostptr reuse mode";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Aligned4KB, EnumType::Misaligned, EnumType::Map};
    const static inline std::string enumValuesNames[3] = {"Aligned4KB", "Misaligned", "Map"};
};
