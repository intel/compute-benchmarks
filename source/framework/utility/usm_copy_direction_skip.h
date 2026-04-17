/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_memory_placement.h"

inline constexpr bool shouldSkipCopyDirection(UsmMemoryPlacement sourcePlacement, UsmMemoryPlacement destinationPlacement) {
    return (isHostMemoryType(sourcePlacement) && isHostMemoryType(destinationPlacement)) ||
           (isDeviceMemoryType(sourcePlacement) && isDeviceMemoryType(destinationPlacement));
}
