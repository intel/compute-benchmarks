/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/enum/device_selection.h"

namespace OCL {
struct ContextProperties {
    DeviceSelection deviceSelection = DeviceSelection::Unknown;
    bool createContext = true;
    bool requireCreationSuccess = true;

    static ContextProperties create() {
        return ContextProperties()
            .setDeviceSelection(Configuration::get().subDeviceSelection);
    }

    ContextProperties &setDeviceSelection(DeviceSelection newValue) {
        this->deviceSelection = newValue;
        return *this;
    }

    ContextProperties &disable() {
        createContext = false;
        return *this;
    }

    ContextProperties &allowCreationFail() {
        requireCreationSuccess = false;
        return *this;
    }
};
} // namespace OCL
