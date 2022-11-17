/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/stream_memory_type.h"

struct StreamMemoryTypeArgument : EnumArgument<StreamMemoryTypeArgument, StreamMemoryType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "stream memory type";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[] = {EnumType::Read, EnumType::Write, EnumType::Scale, EnumType::Triad};
    const static inline EnumType onlyReadAndTriad[] = {EnumType::Read, EnumType::Triad};
    const static inline std::string enumValuesNames[] = {"Read", "Write", "Scale", "Triad"};
};
