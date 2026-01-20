/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct AppendMemCopyArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument useEvent;
    UsmMemoryPlacementArgument src;
    UsmMemoryPlacementArgument dst;
    Uint32Argument appendCount;
    BooleanArgument forceBlitter;

    AppendMemCopyArguments()
        : size(*this, "size", "Size of memory to copy"),
          useEvent(*this, "event", "Pass output event to the append call"),
          src(*this, "src", "Placement of source memory"),
          dst(*this, "dst", "Placement of destination memory"),
          appendCount(*this, "appendCount", "Number of appends to run"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct AppendMemCopy : TestCase<AppendMemCopyArguments> {
    using TestCase<AppendMemCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMemCopy";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendMemoryCopy on CPU.";
    }
};
