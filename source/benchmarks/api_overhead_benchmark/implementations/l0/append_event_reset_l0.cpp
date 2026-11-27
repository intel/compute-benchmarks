/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_event_reset.h"
#include "utility/l0/append_event_utility_l0.h"

static TestResult run(const AppendEventResetArguments &arguments, Statistics &statistics) {
    return measureAppendEvent(arguments.iterations, arguments.appendCount, false, arguments.timestampEvents, statistics,
                              [](ze_command_list_handle_t cmdList, ze_event_handle_t event) {
                                  return zeCommandListAppendEventReset(cmdList, event);
                              });
}

static RegisterTestCaseImplementation<AppendEventReset> registerTestCase(run, Api::L0);
