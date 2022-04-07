/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "supported_apis.h"

void SupportedApis::registerSupportedApi(Api api) {
    auto &slot = supportedApis[static_cast<int>(api)];
    DEVELOPER_WARNING_IF(slot, "Api support registered multiple times");
    slot = true;
}

bool SupportedApis::isApiSupported(Api api) {
    return supportedApis[static_cast<int>(api)];
}
