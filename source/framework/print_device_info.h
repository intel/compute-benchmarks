/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"

struct DeviceInfo {
    using PrintDeviceInfoFunction = void (*)();
    using PrintAvailableDevicesFunction = void (*)();
    static void registerFunctions(Api api, PrintDeviceInfoFunction printDeviceInfo, PrintAvailableDevicesFunction printAvailableDevices);

    static void printDeviceInfo();
    static void printAvailableDevices();

  private:
    struct Functions {
        PrintDeviceInfoFunction printDeviceInfo = nullptr;
        PrintAvailableDevicesFunction printAvailableDevices = nullptr;
    };
    static Functions functions[static_cast<int>(Api::COUNT)];
};
