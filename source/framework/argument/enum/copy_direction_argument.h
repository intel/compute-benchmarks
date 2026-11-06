/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/copy_direction.h"

struct CopyDirectionArgument : EnumArgument<CopyDirectionArgument, CopyDirection> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    static constexpr const char *enumName = "copy direction";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[2] = {EnumType::HostToDevice, EnumType::DeviceToHost};
    static constexpr const char *enumValuesNames[2] = {"H2D", "D2H"};
};
