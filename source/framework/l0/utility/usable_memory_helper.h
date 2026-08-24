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
    // Currently usable device memory, or nothing when the driver does not report it.
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
};
