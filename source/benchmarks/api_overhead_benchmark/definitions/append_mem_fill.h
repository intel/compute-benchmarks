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

struct AppendMemFillArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    ByteSizeArgument patternSize;
    BooleanArgument useEvent;
    UsmMemoryPlacementArgument dst;
    NonNegativeIntegerArgument dstOffset;
    Uint32Argument appendCount;
    BooleanArgument forceBlitter;
    BooleanArgument useImmediate;

    AppendMemFillArguments()
        : size(*this, "size", "Size of memory to fill"),
          patternSize(*this, "patternSize", "Size of the fill pattern"),
          useEvent(*this, "event", "Pass output event to the append call"),
          dst(*this, "dst", "Placement of destination memory"),
          dstOffset(*this, "dstOffset", "Byte offset added to the destination pointer, a non-zero value makes it unaligned"),
          appendCount(*this, "appendCount", "Number of appends to run"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useImmediate(*this, "immediate", "Append to an immediate command list instead of a regular one") {}
};

struct AppendMemFill : TestCase<AppendMemFillArguments> {
    using TestCase<AppendMemFillArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMemFill";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendMemoryFill on CPU, on a regular or an immediate command list.";
    }
};
