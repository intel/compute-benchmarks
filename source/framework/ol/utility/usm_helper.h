/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/usm_memory_placement.h"
#include "framework/enum/usm_runtime_memory_placement.h"

#include <OffloadAPI.h>

namespace OL::UsmHelper {

inline constexpr auto misalignedOffset = 4u;

bool isPlacementSupported(UsmMemoryPlacement placement,
                          ol_device_handle_t hostDevice);

ol_result_t allocate(UsmMemoryPlacement placement, ol_context_handle_t context,
                     ol_device_handle_t device, size_t size, void **ptr);

ol_result_t allocate(UsmRuntimeMemoryPlacement runtimePlacement,
                     ol_context_handle_t context, ol_device_handle_t device,
                     size_t size, void **ptr);

ol_result_t allocateAligned(UsmMemoryPlacement placement,
                            ol_context_handle_t context,
                            ol_device_handle_t device, size_t alignment,
                            size_t size, void **ptr);

ol_result_t deallocate(UsmMemoryPlacement placement,
                       ol_context_handle_t context, void *ptr);

// USM host and shared allocations are owned by the GPU device, so olMemcpy
// takes the GPU device for them, not the host device.
ol_device_handle_t getMemcpyDevice(UsmMemoryPlacement placement,
                                   ol_device_handle_t device,
                                   ol_device_handle_t hostDevice);

} // namespace OL::UsmHelper
