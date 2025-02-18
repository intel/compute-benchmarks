/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ur.h"

#include "error.h"

#include <stdexcept>
#include <vector>

UrState::UrState() {
    ur_device_init_flags_t device_flags = 0;
    EXPECT_UR_RESULT_SUCCESS(urLoaderInit(device_flags, nullptr));

    uint32_t adapter_count = 0;
    EXPECT_UR_RESULT_SUCCESS(urAdapterGet(0, nullptr, &adapter_count));

    if (adapter_count == 0) {
        FATAL_ERROR("No adapters found");
    }

    // if more than one adapter is found, select the first one

    EXPECT_UR_RESULT_SUCCESS(urAdapterGet(1, &adapter, nullptr));

    uint32_t platform_count = 0;
    EXPECT_UR_RESULT_SUCCESS(urPlatformGet(&adapter, 1, 0, nullptr, &platform_count));

    std::vector<ur_platform_handle_t> platforms(platform_count);
    EXPECT_UR_RESULT_SUCCESS(urPlatformGet(&adapter, 1, platform_count, platforms.data(),
                                           nullptr));

    if (Configuration::get().urPlatformIndex >= platform_count) {
        FATAL_ERROR("Invalid UR platform index. platformIndex=",
                    Configuration::get().urPlatformIndex, " platformCount=",
                    platform_count);
    }
    platform = platforms[Configuration::get().urPlatformIndex];

    uint32_t device_count = 0;
    EXPECT_UR_RESULT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, 0, nullptr, &device_count));

    std::vector<ur_device_handle_t> devices(device_count);
    EXPECT_UR_RESULT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, device_count, devices.data(),
                                         nullptr));

    if (Configuration::get().urDeviceIndex >= device_count) {
        FATAL_ERROR("Invalid UR device index. deviceIndex=",
                    Configuration::get().urDeviceIndex, " deviceCount=",
                    device_count);
    }
    device = devices[Configuration::get().urDeviceIndex];

    for (auto &device : devices) {
        if (device != this->device) {
            urDeviceRelease(device);
        }
    }

    EXPECT_UR_RESULT_SUCCESS(urContextCreate(1, &device, nullptr, &context));
}

UrState::~UrState() {
    if (device) {
        urDeviceRelease(device);
    }
    if (context) {
        urContextRelease(context);
    }
    if (adapter) {
        urAdapterRelease(adapter);
    }
    urLoaderTearDown();
}
