/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/usm_memadvise_preferred_location.h"

struct UsmMemAdvisePreferredLocationArgument : EnumArgument<UsmMemAdvisePreferredLocationArgument, UsmMemAdvisePreferredLocation> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "USM memadise preferred location";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::System, EnumType::Device, EnumType::None};
    const static inline std::string enumValuesNames[3] = {"System", "Device", "None"};
};
