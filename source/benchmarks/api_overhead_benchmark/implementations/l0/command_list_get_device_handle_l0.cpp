/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/command_list_get_device_handle.h"
#include "utility/l0/command_list_query_utility_l0.h"

static TestResult run(const CommandListQueryArguments &arguments, Statistics &statistics) {
    return measureCommandListQuery<ze_device_handle_t>(
        arguments, statistics, arguments.useImmediate,
        [](ze_command_list_handle_t cmdList, ze_device_handle_t *device) { return zeCommandListGetDeviceHandle(cmdList, device); },
        [](const L0::LevelZero &levelzero, ze_device_handle_t device) { return device == levelzero.device; });
}

static RegisterTestCaseImplementation<CommandListGetDeviceHandle> registerTestCase(run, Api::L0);
