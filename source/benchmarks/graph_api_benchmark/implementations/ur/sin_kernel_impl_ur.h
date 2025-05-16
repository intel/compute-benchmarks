/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/utility/file_helper.h"

#include "definitions/sin_kernel_graph_base.h"

#include <iostream>
#include <ur_api.h>

class SinKernelGraphUR : public SinKernelGraphBase {
  public:
    SinKernelGraphUR(const SinKernelGraphArguments &arguments)
        : SinKernelGraphBase(arguments), urstate(nullptr), syncPoints(arguments.numKernels + 1) {
    }

    DataFloatPtr allocDevice(uint32_t count) override;
    DataFloatPtr allocHost(uint32_t count) override;

    TestResult init() override;
    TestResult destroy() override;
    TestResult recordGraph() override;
    TestResult readResults(float *output_h) override;
    TestResult runGraph(float *input_h) override;
    TestResult runEager(float *input_h) override;
    TestResult waitCompletion() override;

    TestResult runKernels();

  private:
    std::shared_ptr<UrState> urstate;
    ur_kernel_handle_t kernelAssign;
    ur_kernel_handle_t kernelSin;
    ur_program_handle_t programA;
    ur_program_handle_t programS;

    ur_queue_handle_t queue;
    ur_exp_command_buffer_handle_t cmdBuffer = nullptr;

    // all commands use this single sync point
    std::vector<ur_exp_command_buffer_sync_point_t> syncPoints;
};
