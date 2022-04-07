/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/utility/print_device_info_l0.inl"
#include "framework/print_device_info.h"
#include "framework/supported_apis.h"
#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    DeviceInfo::registerFunctions(Api::L0, L0::printDeviceInfo, L0::printAvailableDevices);
    SupportedApis::registerSupportedApi(Api::L0);
};
