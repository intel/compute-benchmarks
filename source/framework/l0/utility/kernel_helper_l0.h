/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/test_case/test_result.h"

#include <level_zero/ze_api.h>
namespace L0::KernelHelper {
TestResult loadKernel(LevelZero &levelzero, std::string spirv_file_name, std::string kernel_name, ze_kernel_handle_t *kernel_out,
                      ze_module_handle_t *module_out);
}