/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/priority_level.h"

struct PriorityLevelArgument : EnumArgument<PriorityLevelArgument, PriorityLevel> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "Queue priority level";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Low, EnumType::Normal, EnumType::High};
    static constexpr const char *enumValuesNames[3] = {"Low", "Normal", "High"};
};
