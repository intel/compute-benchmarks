/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <cstdint>
#include <level_zero/ze_api.h>

using zeCommandListAppendHostFunctionPF = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList,
                                                                    void *pHostFunction,
                                                                    void *pUserData,
                                                                    void *pNext,
                                                                    ze_event_handle_t hSignalEvent,
                                                                    uint32_t numWaitEvents,
                                                                    ze_event_handle_t *phWaitEvents);

struct HostFunctionApi {
    zeCommandListAppendHostFunctionPF commandListAppendHostFunction = nullptr;
};

inline HostFunctionApi loadHostFunctionApi(ze_driver_handle_t driver) {
    HostFunctionApi ret;
    zeDriverGetExtensionFunctionAddress(driver, "zeCommandListAppendHostFunction", reinterpret_cast<void **>(&ret.commandListAppendHostFunction));
    return ret;
}

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
