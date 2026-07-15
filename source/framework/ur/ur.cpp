/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "ur.h"

#include "error.h"

#include <cstdlib>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace {
std::once_flag urInitOnce;
ur_adapter_handle_t levelZeroAdapter = nullptr;

void teardownUrAtExit() {
    if (levelZeroAdapter) {
        EXPECT_UR_RESULT_SUCCESS(urAdapterRelease(levelZeroAdapter));
        levelZeroAdapter = nullptr;
    }
    EXPECT_UR_RESULT_SUCCESS(urLoaderTearDown());
}

void initializeUrAndSelectAdapter() {
    if (levelZeroAdapter != nullptr) {
        return;
    }

    ur_device_init_flags_t deviceFlags = 0;
    EXPECT_UR_RESULT_SUCCESS(urLoaderInit(deviceFlags, nullptr));

    uint32_t adapterCount = 0;
    EXPECT_UR_RESULT_SUCCESS(urAdapterGet(0, nullptr, &adapterCount));
    if (adapterCount == 0) {
        EXPECT_UR_RESULT_SUCCESS(urLoaderTearDown());
        FATAL_ERROR("No adapters found");
    }

    std::vector<ur_adapter_handle_t> adapters(adapterCount);
    EXPECT_UR_RESULT_SUCCESS(urAdapterGet(adapterCount, adapters.data(), nullptr));

    for (auto candAdapter : adapters) {
        ur_backend_t backend{};
        EXPECT_UR_RESULT_SUCCESS(urAdapterGetInfo(candAdapter, UR_ADAPTER_INFO_BACKEND,
                                                  sizeof(backend), &backend, nullptr));
        if (backend == UR_BACKEND_LEVEL_ZERO) {
            levelZeroAdapter = candAdapter;
            break;
        }
    }

    for (auto &adapter : adapters) {
        if (adapter != levelZeroAdapter) {
            EXPECT_UR_RESULT_SUCCESS(urAdapterRelease(adapter));
        }
    }

    if (levelZeroAdapter == nullptr) {
        EXPECT_UR_RESULT_SUCCESS(urLoaderTearDown());
        FATAL_ERROR("No Level Zero adapter found.");
    }

    std::atexit(teardownUrAtExit);
}

// Initialize loader and select Level Zero adapter only once per process.
// Right now UrState is used once in early benchmark configuration, then again
// in some UR benchmarks. We shouldn't initialize loader twice.
ur_adapter_handle_t getLevelZeroAdapter() {
    std::call_once(urInitOnce, initializeUrAndSelectAdapter);
    EXPECT_UR_RESULT_SUCCESS(urAdapterRetain(levelZeroAdapter));
    return levelZeroAdapter;
}

} // namespace

UrState::UrState() {
    adapter = nullptr;
    platform = nullptr;
    context = nullptr;
    device = nullptr;

    adapter = getLevelZeroAdapter();

    uint32_t platform_count = 0;
    EXPECT_UR_RESULT_SUCCESS(urPlatformGet(adapter, 0, nullptr, &platform_count));

    std::vector<ur_platform_handle_t> platforms(platform_count);
    EXPECT_UR_RESULT_SUCCESS(urPlatformGet(adapter, platform_count, platforms.data(),
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
            EXPECT_UR_RESULT_SUCCESS(urDeviceRelease(device));
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
}
