/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListWithIndirectAccessArguments : TestCaseArgumentContainer {
    IntegerArgument IndirectAllocationsAmount;
    BooleanArgument AllocateMemory;

    ExecuteCommandListWithIndirectAccessArguments()
        : IndirectAllocationsAmount(*this, "AmountOfIndirectAllocations", "Amount of indirect allocations that are present in system"),
          AllocateMemory(*this, "AllocateMemory", "If set then prior to measurement new allocation is done and made resident.") {}
};

struct ExecuteCommandListWithIndirectAccess : TestCase<ExecuteCommandListWithIndirectAccessArguments> {
    using TestCase<ExecuteCommandListWithIndirectAccessArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListWithIndirectAccess";
    }

    std::string getHelp() const override {
        return "measures time spent in zeCommandQueueExecuteCommandLists on CPU when indirect allocations are accessed.";
    }
};
