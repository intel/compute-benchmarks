/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ol/utility/usm_helper.h"

#include "framework/utility/aligned_allocator.h"
#include "framework/utility/error.h"

#include <cstdlib>

namespace OL::UsmHelper {

namespace {
bool isUsm(UsmMemoryPlacement placement) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
    case UsmMemoryPlacement::Host:
    case UsmMemoryPlacement::Shared:
        return true;
    default:
        return false;
    }
}
} // namespace

bool isPlacementSupported(UsmMemoryPlacement placement,
                          ol_device_handle_t hostDevice) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
    case UsmMemoryPlacement::Host:
    case UsmMemoryPlacement::Shared:
        return true;
    case UsmMemoryPlacement::NonUsm:
    case UsmMemoryPlacement::NonUsm4KBAligned:
    case UsmMemoryPlacement::NonUsm2MBAligned:
    case UsmMemoryPlacement::NonUsmMisaligned:
        return hostDevice != nullptr;
    default:
        return false;
    }
}

ol_result_t allocate(UsmMemoryPlacement placement, ol_context_handle_t context,
                     ol_device_handle_t device, size_t size, void **ptr) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
        return olMemAlloc(context, device, OL_ALLOC_TYPE_DEVICE, size, ptr);
    case UsmMemoryPlacement::Host:
        return olMemAllocHost(context, device, size, ptr);
    case UsmMemoryPlacement::Shared:
        return olMemAlloc(context, device, OL_ALLOC_TYPE_MANAGED, size, ptr);
    case UsmMemoryPlacement::NonUsm:
        *ptr = std::malloc(size);
        return OL_SUCCESS;
    case UsmMemoryPlacement::NonUsm4KBAligned:
        *ptr = Allocator::alloc4KBAligned(size);
        return OL_SUCCESS;
    case UsmMemoryPlacement::NonUsm2MBAligned:
        *ptr = Allocator::alloc2MBAligned(size);
        return OL_SUCCESS;
    case UsmMemoryPlacement::NonUsmMisaligned:
        *ptr = Allocator::allocMisaligned(size, misalignedOffset);
        return OL_SUCCESS;
    default:
        FATAL_ERROR("Unknown placement");
    }
}

ol_result_t allocateAligned(UsmMemoryPlacement placement,
                            ol_context_handle_t context,
                            ol_device_handle_t device, size_t alignment,
                            size_t size, void **ptr) {
    switch (placement) {
    case UsmMemoryPlacement::Device:
        return olMemAllocAligned(context, device, OL_ALLOC_TYPE_DEVICE, size,
                                 alignment, ptr);
    default:
        FATAL_ERROR("Unknown placement");
    }
}

ol_result_t deallocate(UsmMemoryPlacement placement,
                       ol_context_handle_t context, void *ptr) {
    switch (placement) {
    case UsmMemoryPlacement::NonUsm:
        std::free(ptr);
        return OL_SUCCESS;
    case UsmMemoryPlacement::NonUsm4KBAligned:
    case UsmMemoryPlacement::NonUsm2MBAligned:
        Allocator::alignedFree(ptr);
        return OL_SUCCESS;
    case UsmMemoryPlacement::NonUsmMisaligned:
        Allocator::misalignedFree(ptr, misalignedOffset);
        return OL_SUCCESS;
    default:
        return olMemFree(context, ptr);
    }
}

ol_device_handle_t getMemcpyDevice(UsmMemoryPlacement placement,
                                   ol_device_handle_t device,
                                   ol_device_handle_t hostDevice) {
    return isUsm(placement) ? device : hostDevice;
}

} // namespace OL::UsmHelper
