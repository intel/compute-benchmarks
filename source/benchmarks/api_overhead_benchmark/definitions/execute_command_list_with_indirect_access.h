/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListWithIndirectAccessArguments : TestCaseArgumentContainer {
    IntegerArgument IndirectAllocationsAmount;

    ExecuteCommandListWithIndirectAccessArguments()
        : IndirectAllocationsAmount(*this, "AmountOfIndirectAllocations", "Amount of indirect allocations that are present in system") {}
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
