/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/test_case/test_case.h"

struct ResetCommandListArguments : TestCaseArgumentContainer {
    UsmMemoryPlacementArgument sourcePlacement;
    ByteSizeArgument size;
    BooleanArgument copyOnly;

    ResetCommandListArguments()
        : sourcePlacement(*this, "sourcePlacement", "Placement of the source buffer"),
          size(*this, "size", "Size of the buffer"),
          copyOnly(*this, "CopyOnly", "Create copy only cmdlist") {}
};

struct ResetCommandList : TestCase<ResetCommandListArguments> {
    using TestCase<ResetCommandListArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ResetCommandList";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandListReset on CPU.";
    }
};
