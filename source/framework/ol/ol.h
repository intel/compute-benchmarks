/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/configuration.h"

#include <OffloadAPI.h>

struct OlInitGuard {
    explicit OlInitGuard(ol_init_args_t *initArgs);
    OlInitGuard(const OlInitGuard &) = delete;
    OlInitGuard &operator=(const OlInitGuard &) = delete;
    ~OlInitGuard();
};

struct OlState {
    OlState();
    OlState(const OlState &) = delete;
    OlState &operator=(const OlState &) = delete;
    ~OlState();

    OlInitGuard olInitGuard;
    ol_context_handle_t context = nullptr;
    ol_device_handle_t device = nullptr;
};
