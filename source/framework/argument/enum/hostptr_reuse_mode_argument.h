/*
 * Copyright (C) 2022-2026 Intel Corporation
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

    static constexpr const char *enumName = "Buffer hostptr reuse mode";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[4] = {EnumType::Aligned4KB, EnumType::Misaligned, EnumType::Usm, EnumType::Map};
    static constexpr const char *enumValuesNames[4] = {"Aligned4KB", "Misaligned", "Usm", "Map"};
};

struct HostptrImageReuseModeArgument : EnumArgument<HostptrImageReuseModeArgument, HostptrReuseMode> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "Image hostptr reuse mode";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Aligned4KB, EnumType::Misaligned, EnumType::Map};
    static constexpr const char *enumValuesNames[3] = {"Aligned4KB", "Misaligned", "Map"};
};
