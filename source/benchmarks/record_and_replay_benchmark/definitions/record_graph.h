/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct RecordGraphArguments : TestCaseArgumentContainer {
    BooleanArgument emulate;

    RecordGraphArguments() : emulate(*this, "emulate", "Emulate record and replay graph API using regular commandlists.") {}
};

struct RecordGraph : TestCase<RecordGraphArguments> {
    using TestCase<RecordGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RecordGraph";
    }

    std::string getHelp() const override {
        return "measures overhead of recording a graph.";
    }
};
