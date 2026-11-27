/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"

#include "definitions/append_signal_event.h"
#include "utility/l0/append_event_utility_l0.h"

static TestResult run(const AppendSignalEventArguments &arguments, Statistics &statistics) {
    return measureAppendEvent(arguments.iterations, arguments.appendCount, arguments.counterBasedEvents, arguments.timestampEvents, statistics,
                              [](ze_command_list_handle_t cmdList, ze_event_handle_t event) {
                                  return zeCommandListAppendSignalEvent(cmdList, event);
                              });
}

static RegisterTestCaseImplementation<AppendSignalEvent> registerTestCase(run, Api::L0);
