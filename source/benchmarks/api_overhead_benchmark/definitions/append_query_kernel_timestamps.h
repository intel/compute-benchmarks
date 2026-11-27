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

struct AppendQueryKernelTimestampsArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument eventCount;
    BooleanArgument useOffsets;
    UsmMemoryPlacementArgument dst;
    BooleanArgument useEvent;
    PositiveIntegerArgument appendCount;

    AppendQueryKernelTimestampsArguments()
        : eventCount(*this, "eventCount", "Number of timestamp events queried by one append"),
          useOffsets(*this, "useOffsets", "Pass an offset for each result, as opposed to writing the results one after another"),
          dst(*this, "dst", "Placement of destination memory"),
          useEvent(*this, "event", "Pass output event to the append call"),
          appendCount(*this, "appendCount", "Number of appends to run") {}
};

struct AppendQueryKernelTimestamps : TestCase<AppendQueryKernelTimestampsArguments> {
    using TestCase<AppendQueryKernelTimestampsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendQueryKernelTimestamps";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListAppendQueryKernelTimestamps on CPU. "
               "The queried events come from an event pool created with ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP.";
    }
};
