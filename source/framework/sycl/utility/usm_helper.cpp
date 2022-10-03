/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/utility/usm_helper.h"

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
    if (placement == UsmMemoryPlacement::NonUsm) {
        std::free(buffer);
    } else {
        sycl::free(buffer, sycl.queue);
    }
}

} // namespace SYCL::UsmHelper
