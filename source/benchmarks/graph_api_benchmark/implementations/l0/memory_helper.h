/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"

#include "definitions/sin_kernel_graph_base.h"

#include <level_zero/ze_api.h>

namespace MemHelper {
using DataFloatPtr = SinKernelGraphBase::DataFloatPtr;
DataFloatPtr alloc(UsmMemoryPlacement placement, std::shared_ptr<LevelZero> levelzero, uint32_t count);
} // namespace MemHelper
