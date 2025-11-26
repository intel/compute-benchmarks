/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include <thread>

template <typename Duration>
void sleep(const Duration &duration) {
    std::this_thread::sleep_for(duration);
}
