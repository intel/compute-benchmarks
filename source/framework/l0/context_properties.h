/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"
#include "framework/enum/device_selection.h"

namespace L0 {
struct ContextProperties {
    DeviceSelection deviceSelection = DeviceSelection::Unknown;
    bool requireCreationSuccess = true;
    bool createContext = true;
    bool fakeSubDeviceAllowed = false;

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

    ContextProperties &createSingleFakeSubDeviceIfNeeded() {
        this->fakeSubDeviceAllowed = true;
        return *this;
    }

    ContextProperties &allowCreationFail() {
        requireCreationSuccess = false;
        return *this;
    }
};
} // namespace L0
