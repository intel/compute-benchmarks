/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <OffloadAPI.h>
#include <ostream>
#include <string>
#include <vector>

namespace OL {

struct DeviceInfoField {
    std::string key;
    std::string value;
    bool showInDeviceListItem = false;
};

std::vector<DeviceInfoField> getDeviceInfo(ol_device_handle_t device);

void printDeviceInfo(std::ostream &output);
void printAvailableDevices();

} // namespace OL
