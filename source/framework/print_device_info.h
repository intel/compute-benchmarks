/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/api.h"

#include <ostream>
#include <string>

struct DeviceInfo {
    using PrintDeviceInfoFunction = void (*)(std::ostream &);
    using PrintAvailableDevicesFunction = void (*)();
    static void registerFunctions(Api api, PrintDeviceInfoFunction printDeviceInfo, PrintAvailableDevicesFunction printAvailableDevices);

    static void printDeviceInfo();
    static std::string getDeviceInfoString();
    static void printAvailableDevices();

  private:
    struct Functions {
        PrintDeviceInfoFunction printDeviceInfo = nullptr;
        PrintAvailableDevicesFunction printAvailableDevices = nullptr;
    };
    static Functions functions[static_cast<int>(Api::COUNT)];
};
