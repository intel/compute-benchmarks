/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/profiler_type.h"

struct ProfilerTypeArgument : EnumArgument<ProfilerTypeArgument, ProfilerType> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "profiler selection";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[2] = {EnumType::Timer, EnumType::CpuCounter};
    const static inline std::string enumValuesNames[2] = {"timer", "cpucounter"};
};
