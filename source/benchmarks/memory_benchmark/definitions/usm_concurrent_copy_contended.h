/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/copy_direction_argument.h"
#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmConcurrentCopyContendedArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    CopyDirectionArgument measuredDirection;
    EngineArgument h2dEngine;
    EngineArgument d2hEngine;
    BooleanArgument useEvents;

    UsmConcurrentCopyContendedArguments()
        : size(*this, "size", "Size of the measured copy and of the copy in the other direction"),
          measuredDirection(*this, "measuredDirection", "Direction of the measured copy"),
          h2dEngine(*this, "h2dEngine", "Engine used for the host to device copy"),
          d2hEngine(*this, "d2hEngine", "Engine used for the device to host copy"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        return h2dEngine != d2hEngine;
    }
};

struct UsmConcurrentCopyContended : TestCase<UsmConcurrentCopyContendedArguments> {
    using TestCase<UsmConcurrentCopyContendedArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmConcurrentCopyContended";
    }

    std::string getHelp() const override {
        return "measures the bandwidth of a copy between unified host and device memory in one direction while a copy "
               "of the same size runs in the other direction on a second engine. Both copies are released together by "
               "one host-signalled event. With useEvents the test also reports 'Overlap', the part of the measured "
               "copy that overlapped the other copy, from GPU timestamps. An overlap below 100% means that for part of "
               "its time the measured copy ran while the other copy was not running.";
    }
};
