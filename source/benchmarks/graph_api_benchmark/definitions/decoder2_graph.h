/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/timer.h"

struct Decoder2GraphArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numTokens;
    BooleanArgument useGraphs;
    BooleanArgument useHostTasks;

    Decoder2GraphArguments()
        : numTokens(*this, "NumTokens", "Number of tokens"),
          useGraphs(*this, "UseGraphs", "Use graphs (1) or execute eagerly (0)"),
          useHostTasks(*this, "UseHostTasks", "Use host task for single graph execution per token (1) or no host tasks for single graph per layer (0)") {}
};

struct Decoder2Graph : TestCase<Decoder2GraphArguments> {
    using TestCase<Decoder2GraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "Decoder2Graph";
    }

    std::string getHelp() const override {
        return "LLM decode proxy benchmark to simulate graph execution with host CPU coordination.";
    }
};
