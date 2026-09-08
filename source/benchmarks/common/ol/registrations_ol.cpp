/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ol/print_device_info.h"
#include "framework/print_device_info.h"
#include "framework/supported_apis.h"
#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    DeviceInfo::registerFunctions(Api::OL, OL::printDeviceInfo,
                                  OL::printAvailableDevices);
    SupportedApis::registerSupportedApi(Api::OL);
};
