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

struct ExecuteCommandListWithIndirectArguments : TestCaseArgumentContainer {
    IntegerArgument IndirectAllocationsAmount;
    UsmMemoryPlacementArgument placement;

    ExecuteCommandListWithIndirectArguments()
        : IndirectAllocationsAmount(*this, "AmountOfIndirectAllocations", "Amount of indirect allocations that are present in system"),
          placement(*this, "placement", "Placement of the indirect allocations") {}
};

struct ExecuteCommandListWithInidrect : TestCase<ExecuteCommandListWithIndirectArguments> {
    using TestCase<ExecuteCommandListWithIndirectArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListWithIndirectArguments";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandQueueExecuteCommandLists on CPU when indirect allocations are used.";
    }
};
