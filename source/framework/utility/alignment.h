/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <cstddef>
#include <cstdint>

static constexpr inline size_t alignUp(size_t before, size_t alignment) {
    auto mask = alignment - 1;
    return (before + mask) & ~mask;
}