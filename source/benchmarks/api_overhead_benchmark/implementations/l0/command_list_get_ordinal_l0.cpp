/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/command_list_get_ordinal.h"
#include "utility/l0/command_list_query_utility_l0.h"

static TestResult run(const CommandListQueryArguments &arguments, Statistics &statistics) {
    return measureCommandListQuery<uint32_t>(
        arguments, statistics, arguments.useImmediate,
        [](ze_command_list_handle_t cmdList, uint32_t *ordinal) { return zeCommandListGetOrdinal(cmdList, ordinal); },
        [](const L0::LevelZero &levelzero, uint32_t ordinal) { return ordinal == levelzero.commandQueueDesc.ordinal; });
}

static RegisterTestCaseImplementation<CommandListGetOrdinal> registerTestCase(run, Api::L0);
