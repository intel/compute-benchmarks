/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/error.h"
#include "framework/test_case/register_test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/file_helper.h"

#include "gtest/gtest.h"
#include "level_zero/zer_api.h"

#include <level_zero/ze_api.h>

// to później nie będzie potrzebne i zamiast tego będzie Kernel
class L0Context {
  public:
    LevelZero l0;
    ze_command_list_handle_t cmdListImmediate_1;
    ze_command_list_handle_t cmdListImmediate_2;

    L0Context();
    ~L0Context();
};

class Kernel {
    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};

public:
    Kernel(LevelZero &l0, const std::string &kernelFileName, const std::string &kernelName);
    ~Kernel();

    ze_kernel_handle_t get() const;

    static TestResult create_kernel(LevelZero &l0,
                                    const std::string &kernelFileName,
                                    const std::string &kernelName,
                                    ze_kernel_handle_t &kernel,
                                    ze_module_handle_t &module);

};

class BatchingLoop {
    ze_command_list_handle_t cmdList;
    size_t batchSize;
public:
    BatchingLoop(ze_command_list_handle_t list, size_t batch);
    ~BatchingLoop();
    
    void checkBatch(size_t i);
};

TestResult create_kernel(LevelZero &l0,
                         const std::string &kernelFileName,
                         const std::string &kernelName,
                         ze_kernel_handle_t &kernel,
                         ze_module_handle_t &module);

template <typename data_type>
TestResult l0_malloc_device(LevelZero &l0, size_t length, data_type *&ptr) {
    if (zeMemAllocDevice(l0.context, &zeDefaultGPUDeviceMemAllocDesc, length * sizeof(data_type), alignof(data_type), l0.device, (void **)&ptr) != ZE_RESULT_SUCCESS) {
        return TestResult::Error;
    }
    return TestResult::Success;
}
