/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>
#include <level_zero/zex_driver.h>
#include <level_zero/zex_event.h>

namespace L0 {
using L0ImportExternalPointer = decltype(&zexDriverImportExternalPointer);
using L0ReleaseImportedPointer = decltype(&zexDriverReleaseImportedPointer);
using L0GetHostPointerBaseAddress = decltype(&zexDriverGetHostPointerBaseAddress);
using L0CounterBasedEventCreate2 = decltype(&zexCounterBasedEventCreate2);

struct ExtensionProperties {
    bool getImportHostPointerFunctions = false;
    bool getCounterBasedCreateFunctions = false;

    static ExtensionProperties create() {
        return ExtensionProperties();
    }

    ExtensionProperties &setImportHostPointerFunctions(bool value) {
        getImportHostPointerFunctions = value;
        return *this;
    }

    ExtensionProperties &setCounterBasedCreateFunctions(bool value) {
        getCounterBasedCreateFunctions = value;
        return *this;
    }
};
} // namespace L0
