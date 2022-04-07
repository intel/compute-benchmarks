/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/bitfield_enum_argument.h"
#include "framework/enum/device_selection.h"

struct MultiDeviceSelectionArgument : BitfieldEnumArgument<MultiDeviceSelectionArgument, DeviceSelection> {
    using BitfieldEnumArgument::BitfieldEnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    const static inline std::string enumName = "multi device selection";
    const static inline EnumType zeroEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[5] = {EnumType::Root, EnumType::Tile0, EnumType::Tile1, EnumType::Tile2, EnumType::Tile3};
    const static inline std::string enumValuesNames[5] = {"Root", "Tile0", "Tile1", "Tile2", "Tile3"};
};

struct MultipleTilesSelectionArgument : BitfieldEnumArgument<MultipleTilesSelectionArgument, DeviceSelection> {
    using BitfieldEnumArgument::BitfieldEnumArgument;
    ThisType &operator=(EnumType newValue) {
        this->value = newValue;
        return *this;
    }

    const static inline std::string enumName = "multiple tiles selection";
    const static inline EnumType zeroEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[4] = {EnumType::Tile0, EnumType::Tile1, EnumType::Tile2, EnumType::Tile3};
    const static inline std::string enumValuesNames[4] = {"Tile0", "Tile1", "Tile2", "Tile3"};
};
