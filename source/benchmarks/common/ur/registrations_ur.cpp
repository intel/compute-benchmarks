/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/print_device_info.h"
#include "framework/supported_apis.h"
#include "framework/ur/print_device_info.h"
#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    DeviceInfo::registerFunctions(Api::UR, UR::printDeviceInfo, UR::printAvailableDevices);
    SupportedApis::registerSupportedApi(Api::UR);
};
