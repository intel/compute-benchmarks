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
TestResult loadKernel(LevelZero &levelzero, const std::string &filePath, const std::string &kernelName, ze_kernel_handle_t *kernel,
                      ze_module_handle_t *module);
}