/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"

#include <ur_api.h>

struct UrState {
    UrState();
    ~UrState();

    ur_adapter_handle_t adapter;
    ur_platform_handle_t platform;
    ur_context_handle_t context;
    ur_device_handle_t device;
};
