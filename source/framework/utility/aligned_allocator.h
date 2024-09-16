/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <cstddef>

constexpr inline size_t alignUp(size_t before, size_t alignment) {
    auto mask = alignment - 1;
    return (before + mask) & ~mask;
}

const auto sizeOf2MB = 2 * 1024 * 1024;

void *alloc2MBAligned(size_t size);