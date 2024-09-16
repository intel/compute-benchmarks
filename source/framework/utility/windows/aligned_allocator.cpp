/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/aligned_allocator.h"

#include "malloc.h"

void *alloc2MBAligned(size_t size) {
    return _aligned_malloc(alignUp(size, sizeOf2MB), sizeOf2MB);
}
