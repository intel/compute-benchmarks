/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"

#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

// ------------------------------------------------------------------------- Helpers for creating/freeing platforms and devices

std::vector<sycl::device> createSubDevices(const sycl::device &rootDevice) {
    auto domain = rootDevice.get_info<sycl::info::device::partition_type_affinity_domain>();

    if (domain != sycl::info::partition_affinity_domain::numa) {
        return {};
    }
    std::vector<sycl::device> result = rootDevice.create_sub_devices<sycl::info::partition_property::partition_by_affinity_domain>(domain);
    return result;
}

// ------------------------------------------------------------------------- Printing functions

void showPlatform(size_t indentLevel, const sycl::platform &platform, size_t platformIndex) {
    const std::string indent0(indentLevel + 0, '\t');

    std::cout << indent0 << "Platform " << platformIndex << ": " << platform.get_info<sycl::info::platform::name>() << '\n';
}

void showDevice(size_t indentLevel, const sycl::device &device, const std::string &deviceLabel, size_t deviceIndex) {
    const std::string indent0(indentLevel + 0, '\t');
    const std::string indent1(indentLevel + 1, '\t');
    const std::string indent2(indentLevel + 2, '\t');

    // Print device header
    std::cout << indent0 << deviceLabel << " " << deviceIndex << ": " << device.get_info<sycl::info::device::name>() << '\n';
}

void showDeviceAndItsSubDevices(size_t indentLevel, size_t deviceLevel, const sycl::device &device, size_t deviceIndex) {
    const std::string deviceNames[] = {
        "RootDevice",
        "SubDevice",
        "SubSubDevice",
    };
    const std::string deviceLabel = deviceNames[deviceLevel];

    showDevice(indentLevel, device, deviceLabel, deviceIndex);

    std::vector<sycl::device> subDevices = createSubDevices(device);
    for (auto subDeviceIndex = 0u; subDeviceIndex < subDevices.size(); subDeviceIndex++) {
        showDeviceAndItsSubDevices(indentLevel + 1, deviceLevel + 1, subDevices[subDeviceIndex], subDeviceIndex);
    }
}

// ------------------------------------------------------------------------- Main procedure

int main() {
    Configuration::loadDefaultConfiguration();

    std::vector<sycl::platform> platforms = sycl::platform::get_platforms();
    for (auto platformIndex = 0u; platformIndex < platforms.size(); platformIndex++) {
        sycl::platform platform = platforms[platformIndex];
        showPlatform(0u, platform, platformIndex);

        std::vector<sycl::device> devices = platform.get_devices();
        for (auto deviceIndex = 0u; deviceIndex < devices.size(); deviceIndex++) {
            sycl::device device = devices[deviceIndex];
            showDeviceAndItsSubDevices(1u, 0u, device, deviceIndex);
        }
    }

    return 0;
}
