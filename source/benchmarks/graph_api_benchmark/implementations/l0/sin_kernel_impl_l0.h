/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/utility/file_helper.h"

#include "definitions/sin_kernel_graph_base.h"

#include <iostream>
#include <level_zero/ze_api.h>
#include <math.h>
#include <memory>

class SinKernelGraphL0 : public SinKernelGraphBase {
  public:
    SinKernelGraphL0(const SinKernelGraphArguments &arguments)
        : SinKernelGraphBase(arguments), levelzero(nullptr) {
    }

    ~SinKernelGraphL0();

    DataFloatPtr allocDevice(uint32_t count) override;
    DataFloatPtr allocHost(uint32_t count) override;

    TestResult init() override;
    TestResult recordGraph() override;
    TestResult readResults(float *output_h) override;
    TestResult runGraph(float *input_h) override;
    TestResult runEager(float *input_h) override;
    TestResult waitCompletion() override;

    TestResult runKernels(ze_command_list_handle_t cmdList);

  private:
    std::shared_ptr<LevelZero> levelzero;

    ze_group_count_t groupCount{64u, 1u, 1u};
    ze_kernel_handle_t kernelAssign{};
    ze_kernel_handle_t kernelSin{};
    ze_module_handle_t moduleAssign{};
    ze_module_handle_t moduleSin{};
    ze_command_list_handle_t graphCmdList{};
    ze_command_list_handle_t immCmdList{};
    ze_command_queue_handle_t cmdQueue{};
    ze_event_handle_t zeEvent{};
    ze_event_pool_handle_t zePool{};
};
