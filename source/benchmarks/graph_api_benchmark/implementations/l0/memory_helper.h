/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */
#pragma once
#include "framework/l0/levelzero.h"

#include "definitions/sin_kernel_graph_base.h"

#include <level_zero/ze_api.h>

namespace mem_helper {
using DataFloatPtr = SinKernelGraphBase::DataFloatPtr;
DataFloatPtr allocDevice(std::shared_ptr<LevelZero> levelzero, uint32_t count);

DataFloatPtr allocHost(std::shared_ptr<LevelZero> levelzero, uint32_t count);
TestResult loadKernel(std::shared_ptr<LevelZero> levelzero, std::string spirv_file_name, std::string kernel_name, ze_kernel_handle_t *kernel_out,
                      ze_module_handle_t *module_out);
} // namespace mem_helper