/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct ExecuteCommandListImmediateCopyQueueArguments : TestCaseArgumentContainer {
    BooleanArgument isCopyOnly;
    BooleanArgument measureCompletionTime;

    ExecuteCommandListImmediateCopyQueueArguments()
        : isCopyOnly(*this, "IsCopyOnly", "If true, Copy Engine is selected. If false, Compute Engine is selected"),
          measureCompletionTime(*this, "MeasureCompletionTime", "Measures time taken to complete the submission (default is to measure only Immediate call)") {}
};

struct ExecuteCommandListImmediateCopyQueue : TestCase<ExecuteCommandListImmediateCopyQueueArguments> {
    using TestCase<ExecuteCommandListImmediateCopyQueueArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ExecuteCommandListImmediateCopyQueue";
    }

    std::string getHelp() const override {
        return "measures time spent in appending memory copy for immediate command list on CPU with Copy Queue.";
    }
};
