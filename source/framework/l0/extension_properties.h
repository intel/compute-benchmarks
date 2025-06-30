/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/ze_api.h>
#include <level_zero/ze_intel_gpu.h>
#include <level_zero/zex_driver.h>
#include <level_zero/zex_event.h>

namespace L0 {
using L0ImportExternalPointer = decltype(&zexDriverImportExternalPointer);
using L0ReleaseImportedPointer = decltype(&zexDriverReleaseImportedPointer);
using L0GetHostPointerBaseAddress = decltype(&zexDriverGetHostPointerBaseAddress);
using L0CounterBasedEventCreate2 = decltype(&zexCounterBasedEventCreate2);
using L0DriverGetDefaultContext = decltype(&zeDriverGetDefaultContext);
using L0AppendLaunchKernelWithArguments = decltype(&zeCommandListAppendLaunchKernelWithArguments);

struct ExtensionProperties {
    bool getImportHostPointerFunctions = false;
    bool getCounterBasedCreateFunctions = false;
    bool getSimplifiedL0Functions = false;

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

    ExtensionProperties &setSimplifiedL0Functions(bool value) {
        getSimplifiedL0Functions = value;
        return *this;
    }
};
} // namespace L0
