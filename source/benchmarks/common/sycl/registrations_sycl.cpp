/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/print_device_info.h"
#include "framework/supported_apis.h"
#include "framework/sycl/sycl.h"
#include "framework/sycl/utility/print_device_info_sycl.inl"
#include "framework/utility/execute_at_app_init.h"

EXECUTE_AT_APP_INIT {
    DeviceInfo::registerFunctions(Api::SYCL, SYCL::printDeviceInfo, SYCL::printAvailableDevices);
    SupportedApis::registerSupportedApi(Api::SYCL);
};
