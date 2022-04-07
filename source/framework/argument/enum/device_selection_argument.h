/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/enum_argument.h"
#include "framework/enum/device_selection.h"

struct DeviceSelectionArgument : EnumArgument<DeviceSelectionArgument, DeviceSelection> {
    using EnumArgument::EnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "device selection";
    const static inline EnumType invalidEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[5] = {EnumType::Root, EnumType::Tile0, EnumType::Tile1, EnumType::Tile2, EnumType::Tile3};
    const static inline std::string enumValuesNames[5] = {"Root", "Tile0", "Tile1", "Tile2", "Tile3"};
};
