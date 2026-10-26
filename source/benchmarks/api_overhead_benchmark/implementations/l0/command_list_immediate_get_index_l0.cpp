/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/command_list_immediate_get_index.h"
#include "utility/l0/command_list_query_utility_l0.h"

static TestResult run(const CommandListImmediateGetIndexArguments &arguments, Statistics &statistics) {
    const bool useImmediate = true;
    return measureCommandListQuery<uint32_t>(
        arguments, statistics, useImmediate,
        [](ze_command_list_handle_t cmdList, uint32_t *index) { return zeCommandListImmediateGetIndex(cmdList, index); },
        [](const L0::LevelZero &levelzero, uint32_t index) { return index == levelzero.commandQueueDesc.index; });
}

static RegisterTestCaseImplementation<CommandListImmediateGetIndex> registerTestCase(run, Api::L0);
