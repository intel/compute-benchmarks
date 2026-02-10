/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/map_flags.h"

struct MapFlagsArgument : EnumArgument<MapFlagsArgument, MapFlags> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "map flag";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Read, EnumType::Write, EnumType::WriteInvalidate};
    static constexpr const char *enumValuesNames[3] = {"Read", "Write", "WriteInvalidate"};
};
