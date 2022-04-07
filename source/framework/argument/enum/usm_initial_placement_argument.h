/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/usm_initial_placement.h"

struct UsmInitialPlacementArgument : EnumArgument<UsmInitialPlacementArgument, UsmInitialPlacement> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "USM initial placement";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[3] = {EnumType::Any, EnumType::Host, EnumType::Device};
    const static inline std::string enumValuesNames[3] = {"Any", "Host", "Device"};
};
