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

struct CopyPriorityPreemptionArguments : TestCaseArgumentContainer {
    ByteSizeArgument longCopySize;
    ByteSizeArgument shortCopySize;
    UsmMemoryPlacementArgument src;
    UsmMemoryPlacementArgument dst;
    BooleanArgument forceBlitter;

    CopyPriorityPreemptionArguments()
        : longCopySize(*this, "longCopySize", "Size of normal-priority (long) memory copy"),
          shortCopySize(*this, "shortCopySize", "Size of high-priority (short) memory copy"),
          src(*this, "src", "Placement of source memory"),
          dst(*this, "dst", "Placement of destination memory"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct CopyPriorityPreemption : TestCase<CopyPriorityPreemptionArguments> {
    using TestCase<CopyPriorityPreemptionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopyPriorityPreemption";
    }

    std::string getHelp() const override {
        return "submits a long memory copy on normal-priority queue, then a short copy on high-priority queue, "
               "and measures GPU time from normal-priority copy start to high-priority copy end.";
    }
};
