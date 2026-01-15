/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <level_zero/driver_experimental/public/zex_graph.h>
#include <level_zero/ze_api.h>
#include <level_zero/ze_intel_gpu.h>
#include <level_zero/zex_driver.h>
#include <level_zero/zex_event.h>

namespace L0 {
using L0ImportExternalPointer = decltype(&zexDriverImportExternalPointer);
using L0ReleaseImportedPointer = decltype(&zexDriverReleaseImportedPointer);
using L0GetHostPointerBaseAddress = decltype(&zexDriverGetHostPointerBaseAddress);
using L0CounterBasedEventCreate2 = decltype(&zexCounterBasedEventCreate2);
using L0GraphCreate = decltype(&zeGraphCreateExp);
using L0CommandListBeginCaptureIntoGraph = decltype(&zeCommandListBeginCaptureIntoGraphExp);
using L0CommandListEndGraphCapture = decltype(&zeCommandListEndGraphCaptureExp);
using L0CommandListInstantiateGraph = decltype(&zeCommandListInstantiateGraphExp);
using L0CommandListAppendGraph = decltype(&zeCommandListAppendGraphExp);
using L0GraphDestroy = decltype(&zeGraphDestroyExp);
using L0ExecutableGraphDestroy = decltype(&zeExecutableGraphDestroyExp);
using L0CommandListAppendHostFunction = ze_result_t(ZE_APICALL *)(ze_command_list_handle_t hCommandList,
                                                                  void *pHostFunction,
                                                                  void *pUserData,
                                                                  void *pNext,
                                                                  ze_event_handle_t hSignalEvent,
                                                                  uint32_t numWaitEvents,
                                                                  ze_event_handle_t *phWaitEvents);

struct ExtensionProperties {
    bool getImportHostPointerFunctions = false;
    bool getCounterBasedCreateFunctions = false;
    bool getGraphFunctions = false;
    bool getHostFunctionFunctions = false;

    static ExtensionProperties create() {
        return ExtensionProperties();
    }

    ExtensionProperties &setImportHostPointerFunctions(bool value) {
        getImportHostPointerFunctions = value;
        return *this;
    }

    ExtensionProperties &setCounterBasedCreateFunctions(bool value) {
        getCounterBasedCreateFunctions = value;
        return *this;
    }

    ExtensionProperties &setGraphFunctions(bool value) {
        getGraphFunctions = value;
        return *this;
    }

    ExtensionProperties &setHostFunctionFunctions(bool value) {
        getHostFunctionFunctions = value;
        return *this;
    }
};
} // namespace L0
