/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_memory_placement.h"
#include "framework/utility/error.h"
#include "framework/utility/memory_constants.h"

#include <string>

namespace AppendMemoryTestNames {

inline std::string size(size_t size) {
    if (size >= MemoryConstants::gigaByte && size % MemoryConstants::gigaByte == 0) {
        return std::to_string(size / MemoryConstants::gigaByte) + "GB";
    }
    if (size >= MemoryConstants::megaByte && size % MemoryConstants::megaByte == 0) {
        return std::to_string(size / MemoryConstants::megaByte) + "MB";
    }
    return std::to_string(size) + "B";
}

inline std::string placement(UsmMemoryPlacement placement) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
        return "Device";
    case UsmMemoryPlacement::Host:
        return "Host";
    case UsmMemoryPlacement::Shared:
        return "Shared";
    default:
        FATAL_ERROR("Unsupported placement in test name");
    }
}

inline std::string engine(bool forceBlitter) {
    return forceBlitter ? "BCS" : "CCS";
}

inline std::string commandList(bool useImmediate) {
    return useImmediate ? "Immediate" : "Regular";
}

} // namespace AppendMemoryTestNames
