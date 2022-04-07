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
#include "framework/l0/levelzero.h"

namespace L0::UsmHelper {

ze_result_t allocate(UsmMemoryPlacement placement, LevelZero &levelZero, size_t size, void **buffer);

ze_result_t allocate(UsmRuntimeMemoryPlacement runtimePlacement, LevelZero &levelZero, size_t size, void **buffer);

ze_result_t deallocate(UsmMemoryPlacement placement, LevelZero &levelZero, void *buffer);

ze_result_t allocate(DeviceSelection placement, LevelZero &levelzero, size_t size, void **outBuffer);

} // namespace L0::UsmHelper
