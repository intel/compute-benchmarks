/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/aligned_allocator.h"

#include "malloc.h"

void *Allocator::alloc4KBAligned(size_t size) {
    return _aligned_malloc(alignUp(size, sizeOf4KB), sizeOf4KB);
}

void *Allocator::alloc2MBAligned(size_t size) {
    return _aligned_malloc(alignUp(size, sizeOf2MB), sizeOf2MB);
}

void Allocator::alignedFree(void *ptr) {
    _aligned_free(ptr);
}
