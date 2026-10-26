/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/command_list_get_context_handle.h"
#include "utility/l0/command_list_query_utility_l0.h"

static TestResult run(const CommandListQueryArguments &arguments, Statistics &statistics) {
    return measureCommandListQuery<ze_context_handle_t>(
        arguments, statistics, arguments.useImmediate,
        [](ze_command_list_handle_t cmdList, ze_context_handle_t *context) { return zeCommandListGetContextHandle(cmdList, context); },
        [](const L0::LevelZero &levelzero, ze_context_handle_t context) { return context == levelzero.context; });
}

static RegisterTestCaseImplementation<CommandListGetContextHandle> registerTestCase(run, Api::L0);
