/*
 * Copyright (C) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/aligned_allocator.h"

#include <stdlib.h>
#include <sys/mman.h>

void *alloc2MBAligned(size_t size) {
    auto alignedSize = alignUp(size, sizeOf2MB);
    auto buffer = std::aligned_alloc(sizeOf2MB, alignedSize);
    madvise(buffer, alignedSize, MADV_HUGEPAGE);
    return buffer;
}
