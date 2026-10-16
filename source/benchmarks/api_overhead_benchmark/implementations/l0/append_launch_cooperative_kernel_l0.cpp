/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_launch_cooperative_kernel.h"
#include "utility/l0/append_launch_kernel_utility_l0.h"

static TestResult run(const AppendLaunchKernelArguments &arguments, Statistics &statistics) {
    const bool cooperativeKernel = true;
    return measureAppendLaunchKernel(arguments, statistics, cooperativeKernel,
                                     [](ze_command_list_handle_t cmdList, ze_kernel_handle_t kernel, const ze_group_count_t *dispatchTraits, ze_event_handle_t event) {
                                         return zeCommandListAppendLaunchCooperativeKernel(cmdList, kernel, dispatchTraits, event, 0, nullptr);
                                     });
}

static RegisterTestCaseImplementation<AppendLaunchCooperativeKernel> registerTestCase(run, Api::L0);
