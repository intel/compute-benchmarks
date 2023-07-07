/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct EnqueueBarrierWithEmptyWaitlistArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument enqueueCount;
    BooleanArgument outOfOrderQueue;

    EnqueueBarrierWithEmptyWaitlistArguments()
        : enqueueCount(*this, "enqueueCount", "Number of enqueues"),
          outOfOrderQueue(*this, "outOfOrderQueue", "Use out of order queue") {}
};

struct EnqueueBarrierWithEmptyWaitlist : TestCase<EnqueueBarrierWithEmptyWaitlistArguments> {
    using TestCase<EnqueueBarrierWithEmptyWaitlistArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EnqueueBarrierWithEmptyWaitlist";
    }

    std::string getHelp() const override {
        return "enqueues kernel with barriers with empty waitlists inbetween, waiting on the last barriers event";
    }
};
