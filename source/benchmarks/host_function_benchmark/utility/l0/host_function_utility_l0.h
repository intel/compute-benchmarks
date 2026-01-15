/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstdint>
#include <level_zero/ze_api.h>

inline void emptyFunction([[maybe_unused]] void *pUserData) {
}

inline void busyLoopFor1msFunction([[maybe_unused]] void *pUserData) {
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();

    while (start + std::chrono::milliseconds(1) > std::chrono::steady_clock::now()) {
    }
}

struct HostFunctions {
    void *function = nullptr;
    void *userData = nullptr;
};

inline HostFunctions getHostFunctions(bool useEmpty) {
    HostFunctions ret;
    if (useEmpty) {
        ret.function = reinterpret_cast<void *>(&emptyFunction);
        ret.userData = nullptr;
    } else {
        ret.function = reinterpret_cast<void *>(&busyLoopFor1msFunction);
        ret.userData = nullptr;
    }

    return ret;
}
