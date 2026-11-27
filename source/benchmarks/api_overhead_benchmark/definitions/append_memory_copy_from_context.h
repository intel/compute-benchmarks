/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/argument/enum/usm_runtime_memory_placement_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct AppendMemoryCopyFromContextArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument useEvent;
    UsmRuntimeMemoryPlacementArgument src;
    UsmMemoryPlacementArgument dst;
    PositiveIntegerArgument appendCount;
    BooleanArgument forceBlitter;

    AppendMemoryCopyFromContextArguments()
        : size(*this, "size", "Size of memory to copy"),
          useEvent(*this, "event", "Pass output event to the append call"),
          src(*this, "src", "Placement of source memory, allocated in the source context"),
          dst(*this, "dst", "Placement of destination memory"),
          appendCount(*this, "appendCount", "Number of appends to run"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct AppendMemoryCopyFromContext : TestCase<AppendMemoryCopyFromContextArguments> {
    using TestCase<AppendMemoryCopyFromContextArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMemoryCopyFromContext";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendMemoryCopyFromContext on CPU. "
               "The source memory is allocated in a second context of the same driver, the destination memory in the context of the command list.";
    }
};
