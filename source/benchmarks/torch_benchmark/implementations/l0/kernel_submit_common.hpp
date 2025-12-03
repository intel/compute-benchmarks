/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"

#include "../../../../../third_party/level-zero-sdk/include/level_zero/ze_api.h"
#include "../../../../framework/l0/utility/error.h"
#include "../../../../framework/test_case/test_result.h"
#include "gtest/gtest.h"
#include "level_zero/zer_api.h"

#include <level_zero/ze_api.h>

struct L0CommonContext {
    ze_driver_handle_t driver;
    ze_context_handle_t context;
    ze_device_handle_t device;
};

TestResult init_level_zero_common(L0CommonContext &ctx);

TestResult create_kernel(L0CommonContext &ctx,
                         const std::string &kernelName,
                         ze_kernel_handle_t &kernel);

template <typename data_type>
data_type *l0_malloc_device(L0CommonContext &ctx,
                            size_t length) {
    data_type *ptr = nullptr;
    if (zeMemAllocDevice(ctx.context, &zeDefaultGPUDeviceMemAllocDesc, length * sizeof(data_type), alignof(data_type), ctx.device, (void **)&ptr) != ZE_RESULT_SUCCESS) {
        return nullptr;
    }
    return ptr;
}
