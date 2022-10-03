/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/device_selection.h"
#include "framework/enum/usm_memory_placement.h"
#include "framework/enum/usm_runtime_memory_placement.h"
#include "framework/sycl/sycl.h"

namespace SYCL::UsmHelper {

void *allocate(UsmMemoryPlacement placement, const Sycl &sycl, size_t size);

void *allocate(UsmRuntimeMemoryPlacement runtimePlacement, const Sycl &sycl, size_t size);

void *allocateAligned(UsmMemoryPlacement placement, const Sycl &sycl, size_t alignment, size_t size);

void *allocateAligned(UsmRuntimeMemoryPlacement placement, const Sycl &sycl, size_t alignment, size_t size);

void deallocate(UsmMemoryPlacement placement, const Sycl &sycl, void *buffer);

} // namespace SYCL::UsmHelper
