/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_launch_kernel_with_parameters.h"
#include "utility/l0/append_launch_kernel_utility_l0.h"

static TestResult run(const AppendLaunchKernelWithParametersArguments &arguments, Statistics &statistics) {
    ze_command_list_append_launch_kernel_param_cooperative_desc_t cooperativeDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_APPEND_PARAM_COOPERATIVE_DESC};
    cooperativeDesc.isCooperative = static_cast<ze_bool_t>(true);
    const void *const pNext = arguments.useCooperative ? &cooperativeDesc : nullptr;

    return measureAppendLaunchKernel(arguments, statistics, arguments.useCooperative,
                                     [pNext](ze_command_list_handle_t cmdList, ze_kernel_handle_t kernel, const ze_group_count_t *dispatchTraits, ze_event_handle_t event) {
                                         return zeCommandListAppendLaunchKernelWithParameters(cmdList, kernel, dispatchTraits, pNext, event, 0, nullptr);
                                     });
}

static RegisterTestCaseImplementation<AppendLaunchKernelWithParameters> registerTestCase(run, Api::L0);
