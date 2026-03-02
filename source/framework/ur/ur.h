/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"

#if __has_include(<ur/ur_api.h>)
#include <ur/ur_api.h>
#else
#include <ur_api.h>
#endif

struct UrState {
    UrState();
    ~UrState();

    ur_adapter_handle_t adapter;
    ur_platform_handle_t platform;
    ur_context_handle_t context;
    ur_device_handle_t device;
};
