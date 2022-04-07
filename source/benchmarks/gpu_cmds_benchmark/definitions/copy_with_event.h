/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct CopyWithEventArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;
    BooleanArgument useHostSignalEvent;
    BooleanArgument useDeviceWaitEvent;
    BooleanArgument useTimestampEvent;

    CopyWithEventArguments()
        : measuredCommands(*this, "measuredCmds", CommonHelpMessage::measuredCommandsCount()),
          useHostSignalEvent(*this, "hostSignalEvent", "Use ZE_EVENT_POOL_HOST_VISIBLE for ze_event_pool_desc_t::flags,"
                                                       " and use ZE_EVENT_SCOPE_FLAG_HOST for ze_event_desc_t::signal"),
          useDeviceWaitEvent(*this, "devWaitEvent", "Use ZE_EVENT_SCOPE_FLAG_DEVICE for ze_event_desc_t::wait"),
          useTimestampEvent(*this, "timestampEvent", "Use ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP for ze_event_pool_desc_t::flags") {}
};

class CopyWithEvent : public TestCase<CopyWithEventArguments> {
  public:
    using TestCase<CopyWithEventArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyWithEvent";
    }

    std::string getHelp() const override {
        return "measures time required to run a copy kernel with various event configurations.";
    }
};
