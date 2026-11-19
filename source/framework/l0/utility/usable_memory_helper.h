/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstdint>
#include <level_zero/ze_api.h>
#include <optional>

struct UsableMemoryHelper {
    static std::optional<uint64_t> query(ze_device_handle_t device) {
        ze_device_usablemem_size_ext_properties_t usableMemProperties{ZE_STRUCTURE_TYPE_DEVICE_USABLEMEM_SIZE_EXT_PROPERTIES};
        ze_device_properties_t deviceProperties{ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES, &usableMemProperties};
        if (zeDeviceGetProperties(device, &deviceProperties) != ZE_RESULT_SUCCESS) {
            return {};
        }
        if (usableMemProperties.currUsableMemSize == 0u) {
            return {};
        }
        return usableMemProperties.currUsableMemSize;
    }

    static std::optional<uint64_t> queryOrTotal(ze_device_handle_t device) {
        if (const auto usableMemSize = query(device); usableMemSize.has_value()) {
            return usableMemSize;
        }
        uint32_t memPropertiesCount = 1;
        ze_device_memory_properties_t memProperties{ZE_STRUCTURE_TYPE_DEVICE_MEMORY_PROPERTIES};
        if (zeDeviceGetMemoryProperties(device, &memPropertiesCount, &memProperties) != ZE_RESULT_SUCCESS) {
            return {};
        }
        return memProperties.totalSize;
    }
};
