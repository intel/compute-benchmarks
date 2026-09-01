/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/print_device_info.h"
#include "framework/supported_apis.h"
#include "framework/utility/execute_at_app_init.h"
#include "framework/vk/utility/print_device_info_vk.inl"

EXECUTE_AT_APP_INIT {
    DeviceInfo::registerFunctions(Api::Vulkan, VK::printDeviceInfo, VK::printAvailableDevices);
    SupportedApis::registerSupportedApi(Api::Vulkan);
};
