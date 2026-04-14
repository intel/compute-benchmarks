/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/utility/usm_helper.h"

#include "framework/utility/aligned_allocator.h"

namespace SYCL::UsmHelper {

void *allocate(UsmMemoryPlacement placement, const Sycl &sycl, size_t size) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
        return sycl::malloc_device(size, sycl.queue);
    case UsmMemoryPlacement::Host:
        return sycl::malloc_host(size, sycl.queue);
    case UsmMemoryPlacement::Shared:
        return sycl::malloc_shared(size, sycl.queue);
    case UsmMemoryPlacement::NonUsm:
        return std::malloc(size);
    case UsmMemoryPlacement::NonUsm4KBAligned:
        return Allocator::alloc4KBAligned(size);
    case UsmMemoryPlacement::NonUsm2MBAligned:
        return Allocator::alloc2MBAligned(size);
    case UsmMemoryPlacement::NonUsmMisaligned:
        return Allocator::allocMisaligned(size, misalignedOffset);
    default:
        FATAL_ERROR("Unknown placement");
    }
}

void *allocate(UsmRuntimeMemoryPlacement runtimePlacement, const Sycl &sycl, size_t size) {
    UsmMemoryPlacement placement{};
    switch (runtimePlacement) {
    case UsmRuntimeMemoryPlacement::Device:
        placement = UsmMemoryPlacement::Device;
        break;
    case UsmRuntimeMemoryPlacement::Host:
        placement = UsmMemoryPlacement::Host;
        break;
    case UsmRuntimeMemoryPlacement::Shared:
        placement = UsmMemoryPlacement::Shared;
        break;
    default:
        FATAL_ERROR("Unknown placement");
    }
    return allocate(placement, sycl, size);
}

void *allocateAligned(UsmMemoryPlacement placement, const Sycl &sycl, size_t alignment, size_t size) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
        return sycl::aligned_alloc_device(alignment, size, sycl.queue);
    case UsmMemoryPlacement::Host:
        return sycl::aligned_alloc_host(alignment, size, sycl.queue);
    case UsmMemoryPlacement::Shared:
        return sycl::aligned_alloc_shared(alignment, size, sycl.queue);
    case UsmMemoryPlacement::NonUsm:
        return std::aligned_alloc(alignment, size);
    default:
        FATAL_ERROR("Unknown placement");
    }
}

void *allocateAligned(UsmRuntimeMemoryPlacement runtimePlacement, const Sycl &sycl, size_t alignment, size_t size) {
    UsmMemoryPlacement placement{};
    switch (runtimePlacement) {
    case UsmRuntimeMemoryPlacement::Device:
        placement = UsmMemoryPlacement::Device;
        break;
    case UsmRuntimeMemoryPlacement::Host:
        placement = UsmMemoryPlacement::Host;
        break;
    case UsmRuntimeMemoryPlacement::Shared:
        placement = UsmMemoryPlacement::Shared;
        break;
    default:
        FATAL_ERROR("Unknown placement");
    }
    return allocateAligned(placement, sycl, alignment, size);
}

void deallocate(UsmMemoryPlacement placement, const Sycl &sycl, void *buffer) {
    switch (placement) {
    case UsmMemoryPlacement::NonUsm:
        std::free(buffer);
        break;
    case UsmMemoryPlacement::NonUsm4KBAligned:
    case UsmMemoryPlacement::NonUsm2MBAligned:
        Allocator::alignedFree(buffer);
        break;
    case UsmMemoryPlacement::NonUsmMisaligned:
        Allocator::misalignedFree(buffer, misalignedOffset);
        break;
    default:
        sycl::free(buffer, sycl.queue);
        break;
    }
}

} // namespace SYCL::UsmHelper
