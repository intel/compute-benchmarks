/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/abstract/bitfield_enum_argument.h"
#include "framework/enum/device_selection.h"

struct DeviceSelectionArgumentBaseTraits {
    using EnumType = DeviceSelection;
    const static inline std::string enumName = "usm device selection";
    const static inline EnumType zeroEnumValue = EnumType::Unknown;
    const static inline EnumType enumValues[6] = {EnumType::Host, EnumType::Root, EnumType::Tile0, EnumType::Tile1, EnumType::Tile2, EnumType::Tile3};
    const static inline std::string enumValuesNames[6] = {"Host", "Root", "Tile0", "Tile1", "Tile2", "Tile3"};
};

template <bool allowHost, bool allowDevice, bool allowShared>
struct UsmDeviceSelectionArgumentBase : BitfieldEnumArgument<DeviceSelectionArgumentBaseTraits, DeviceSelection> {
    using BitfieldEnumArgument::BitfieldEnumArgument;

    UsmDeviceSelectionArgumentBase &operator=(EnumType newValue) {
        this->value = newValue;
        markAsParsed();
        return *this;
    }

    bool validateExtra() const override {
        // Usm can have only one GPU device
        const auto gpuDevices = DeviceSelectionHelper::withoutHost(this->value);
        const auto gpuDevicesCount = DeviceSelectionHelper::getDevicesCount(gpuDevices);
        if (gpuDevicesCount > 1) {
            return false;
        }

        const bool hasHost = DeviceSelectionHelper::hasDevice(this->value, DeviceSelection::Host);
        const bool hasDevice = gpuDevicesCount == 1;
        if (hasHost && hasDevice) {
            return allowShared;
        }

        if (hasHost) {
            return allowHost;
        }

        if (hasDevice) {
            return allowDevice;
        }

        FATAL_ERROR("Unreachable code in UsmDeviceSelectionArgumentBase");
    }
};

using UsmDeviceSelectionArgument = UsmDeviceSelectionArgumentBase<true, true, true>;
using UsmSharedDeviceSelectionArgument = UsmDeviceSelectionArgumentBase<false, false, true>;
