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

struct KernelWithEventArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument measuredCommands;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanArgument useHostSignalEvent;
    BooleanArgument useDeviceWaitEvent;
    BooleanArgument useTimestampEvent;

    KernelWithEventArguments()
        : measuredCommands(*this, "measuredCmds", CommonHelpMessage::measuredCommandsCount()),
          workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)"),
          useHostSignalEvent(*this, "hostSignalEvent", "Use ZE_EVENT_POOL_HOST_VISIBLE for ze_event_pool_desc_t::flags,"
                                                       " and use ZE_EVENT_SCOPE_FLAG_HOST for ze_event_desc_t::signal"),
          useDeviceWaitEvent(*this, "devWaitEvent", "Use ZE_EVENT_SCOPE_FLAG_DEVICE for ze_event_desc_t::wait"),
          useTimestampEvent(*this, "timestampEvent", "Use ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP for ze_event_pool_desc_t::flags") {}
};

class KernelWithEvent : public TestCase<KernelWithEventArguments> {
  public:
    using TestCase<KernelWithEventArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelWithEvent";
    }

    std::string getHelp() const override {
        return "measures time required to run an empty kernel with various event configurations.";
    }
};
