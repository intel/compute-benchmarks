/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#define NOMINMAX
#include <Windows.h>

#include <thread>
#include <timeapi.h>

template <typename Duration>
void sleep(const Duration &duration) {
    timeBeginPeriod(1u);
    std::this_thread::sleep_for(duration);
    timeEndPeriod(1u);
}
