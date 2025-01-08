/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_runtime_memory_placement.h"

#include "ur.h"

namespace UR::UsmHelper {

static ur_result_t allocate(UsmRuntimeMemoryPlacement placement, ur_context_handle_t context, ur_device_handle_t device, size_t size, void **ptr) {
    switch (placement) {
    case UsmRuntimeMemoryPlacement::Host:
        return urUSMDeviceAlloc(context, device, nullptr, nullptr, size, ptr);
    case UsmRuntimeMemoryPlacement::Device:
        return urUSMHostAlloc(context, nullptr, nullptr, size, ptr);
    case UsmRuntimeMemoryPlacement::Shared:
        return urUSMSharedAlloc(context, device, nullptr, nullptr, size, ptr);
    default:
        return UR_RESULT_ERROR_INVALID_OPERATION;
    }
}

} // namespace UR::UsmHelper

// urUSMDeviceAlloc()
