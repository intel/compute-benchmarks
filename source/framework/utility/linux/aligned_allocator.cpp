/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/aligned_allocator.h"

#include <stdlib.h>
#include <sys/mman.h>

void *Allocator::alloc4KBAligned(size_t size) {
    auto alignedSize = alignUp(size, sizeOf4KB);
    auto buffer = std::aligned_alloc(sizeOf4KB, alignedSize);
    return buffer;
}

void *Allocator::alloc2MBAligned(size_t size) {
    auto alignedSize = alignUp(size, sizeOf2MB);
    auto buffer = std::aligned_alloc(sizeOf2MB, alignedSize);
    madvise(buffer, alignedSize, MADV_HUGEPAGE);
    return buffer;
}

void Allocator::alignedFree(void *ptr) {
    std::free(ptr);
}
