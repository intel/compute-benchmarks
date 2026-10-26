/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/command_list_is_immediate.h"
#include "utility/l0/command_list_query_utility_l0.h"

static TestResult run(const CommandListQueryArguments &arguments, Statistics &statistics) {
    const bool useImmediate = arguments.useImmediate;
    return measureCommandListQuery<ze_bool_t>(
        arguments, statistics, useImmediate,
        [](ze_command_list_handle_t cmdList, ze_bool_t *isImmediate) { return zeCommandListIsImmediate(cmdList, isImmediate); },
        [useImmediate](const L0::LevelZero &, ze_bool_t isImmediate) { return (isImmediate != 0) == useImmediate; });
}

static RegisterTestCaseImplementation<CommandListIsImmediate> registerTestCase(run, Api::L0);
