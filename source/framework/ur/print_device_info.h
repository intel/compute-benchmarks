/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "error.h"
#include "ur.h"

#include <ur_print.hpp>

namespace UR {

inline std::string stripPrefix(std::string_view value,
                               std::string_view prefix) {
    if (std::equal(prefix.begin(), prefix.end(), value.begin(),
                   value.begin() + std::min(value.size(), prefix.size()))) {
        value.remove_prefix(prefix.size());
    }
    return std::string(value);
}

inline std::string getDeviceType(ur_device_handle_t device) {
    ur_device_type_t deviceType;
    EXPECT_UR_RESULT_SUCCESS(urDeviceGetInfo(device, UR_DEVICE_INFO_TYPE,
                                             sizeof(ur_device_type_t), &deviceType, nullptr));
    std::stringstream deviceTypeStream;
    deviceTypeStream << deviceType;
    std::string deviceTypeStr =
        stripPrefix(deviceTypeStream.str(), "UR_DEVICE_TYPE_");
    std::transform(deviceTypeStr.begin(), deviceTypeStr.end(),
                   deviceTypeStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return deviceTypeStr;
}

template <typename Type, typename Fn, typename Handle, typename Info>
Type getInfo(Handle handle, Fn &&getInfoCb, Info info) {
    if constexpr (std::is_same_v<Type, std::string>) {
        size_t paramSize = 0;
        EXPECT_UR_RESULT_SUCCESS(getInfoCb(handle, info, 0, nullptr,
                                           &paramSize));

        std::string result(paramSize, '\0');
        EXPECT_UR_RESULT_SUCCESS(getInfoCb(handle, info,
                                           paramSize, result.data(),
                                           &paramSize));
        return result;
    } else {
        Type result;
        EXPECT_UR_RESULT_SUCCESS(getInfoCb(handle, info,
                                           sizeof(Type), &result,
                                           nullptr));
        return result;
    }
}

std::string getDeviceName(ur_device_handle_t device) {
    return getInfo<std::string>(device, urDeviceGetInfo, UR_DEVICE_INFO_NAME);
}

std::string getDriverVersion(ur_device_handle_t device) {
    return getInfo<std::string>(device, urDeviceGetInfo, UR_DEVICE_INFO_DRIVER_VERSION);
}

uint32_t getDeviceClockRate(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_MAX_CLOCK_FREQUENCY);
}

uint32_t getDeviceId(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_DEVICE_ID);
}

uint32_t getDeviceVendorId(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_VENDOR_ID);
}

std::string getPlatformVendorName(ur_platform_handle_t platform) {
    return getInfo<std::string>(platform, urPlatformGetInfo, UR_PLATFORM_INFO_VENDOR_NAME);
}

uint32_t getDeviceNumSlices(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_GPU_EU_SLICES);
}

uint32_t getDeviceNumSubslicesPerSlice(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_GPU_SUBSLICES_PER_SLICE);
}

uint32_t getDeviceNumEUPerSubslice(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_GPU_EU_COUNT_PER_SUBSLICE);
}

uint32_t getDeviceNumThreadPerEU(ur_device_handle_t device) {
    return getInfo<uint32_t>(device, urDeviceGetInfo, UR_DEVICE_INFO_GPU_HW_THREADS_PER_EU);
}

ur_adapter_backend_t getAdapterBackend(ur_adapter_handle_t adapter) {
    return getInfo<ur_adapter_backend_t>(adapter, urAdapterGetInfo, UR_ADAPTER_INFO_BACKEND);
}

void printDeviceInfo() {
    UrState ur;

    std::cout << "Adapter: " << getAdapterBackend(ur.adapter) << std::endl;
    std::cout << "  Platform: " << getPlatformVendorName(ur.platform) << std::endl;

    std::cout << "\tDevice: " << getDeviceName(ur.device) << std::endl;
    std::cout << "\t\tType: " << getDeviceType(ur.device) << std::endl;
    std::cout << "\t\tvendorId:     0x" << std::hex << getDeviceVendorId(ur.device) << std::endl;
    std::cout << "\t\tdeviceId:     0x" << std::hex << getDeviceId(ur.device) << std::endl;
    std::cout << "\t\tclockFreq:    " << std::dec << getDeviceClockRate(ur.device) << std::endl;
    std::cout << "\t\tconfig:       " << getDeviceNumSlices(ur.device) << "x" << getDeviceNumSubslicesPerSlice(ur.device) << "x" << getDeviceNumEUPerSubslice(ur.device) << std::endl;
    std::cout << "\t\teuCount:      " << getDeviceNumSlices(ur.device) * getDeviceNumSubslicesPerSlice(ur.device) * getDeviceNumEUPerSubslice(ur.device) << std::endl;
    std::cout << "\t\tthreadsPerEu: " << getDeviceNumThreadPerEU(ur.device) << std::endl;

    std::cout << std::endl;
}

static void printAvailableDevices() {
    UrState ur;

    uint32_t platform_count = 0;
    EXPECT_UR_RESULT_SUCCESS(urPlatformGet(ur.adapter, 0, nullptr, &platform_count));

    std::vector<ur_platform_handle_t> platforms(platform_count);
    EXPECT_UR_RESULT_SUCCESS(urPlatformGet(ur.adapter, platform_count, platforms.data(),
                                           nullptr));

    std::cout << "UR platforms: " << platform_count << std::endl;
    std::cout << "Adapter: " << getAdapterBackend(ur.adapter) << std::endl;

    for (uint32_t platform_index = 0; platform_index < platform_count; platform_index++) {
        auto &platform = platforms[platform_index];
        std::cout << "  Platform: " << getPlatformVendorName(platform) << std::endl;

        uint32_t device_count = 0;
        EXPECT_UR_RESULT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, 0, nullptr, &device_count));

        std::vector<ur_device_handle_t> devices(device_count);
        EXPECT_UR_RESULT_SUCCESS(urDeviceGet(platform, UR_DEVICE_TYPE_ALL, device_count, devices.data(),
                                             nullptr));

        for (uint32_t device_index = 0; device_index < device_count; device_index++) {
            auto &device = devices[device_index];
            std::cout << "    Device: " << getDeviceName(device) << std::endl;
            std::cout << "      Type: " << getDeviceType(ur.device) << std::endl;
            std::cout << "      select this device with --urPlatformIndex=" << platform_index << " --urDeviceIndex=" << device_index;
            std::cout << std::endl;
        }

        for (auto &device : devices) {
            urDeviceRelease(device);
        }
    }

    std::cout << std::endl;
}

} // namespace UR
