/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <cstddef>
#include <cstdint>

struct Allocator {
    static constexpr inline size_t alignUp(size_t before, size_t alignment) {
        auto mask = alignment - 1;
        return (before + mask) & ~mask;
    }

    static constexpr size_t sizeOf4KB = 4u * 1024u;

    static constexpr size_t sizeOf2MB = 2u * 1024u * 1024u;

    static void *alloc4KBAligned(size_t size);

    static void *alloc2MBAligned(size_t size);

    static void alignedFree(void *ptr);

    static void *offsetPointer(void *ptr, int64_t offset) {
        return reinterpret_cast<unsigned char *>(ptr) + offset;
    }

    static void *allocMisaligned(size_t size, int64_t offset) {
        void *alignedPtr = alloc4KBAligned(size + offset);
        return offsetPointer(alignedPtr, offset);
    }

    static void misalignedFree(void *ptr, int64_t offset) {
        void *basePtr = offsetPointer(ptr, offset * -1);
        alignedFree(basePtr);
    }
};