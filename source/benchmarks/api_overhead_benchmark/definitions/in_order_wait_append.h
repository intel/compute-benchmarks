/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/event_scope_argument.h"
#include "framework/test_case/test_case.h"

struct InOrderWaitAppendArguments : TestCaseArgumentContainer {
    BooleanArgument counterBasedEvents;

        InOrderWaitAppendArguments() : counterBasedEvents(*this, "counterBasedEvents", "Use Counter Based Events") {}
};

struct InOrderWaitAppend : TestCase<InOrderWaitAppendArguments> {
    using TestCase<InOrderWaitAppendArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "InOrderWaitAppend";
    }

    std::string getHelp() const override {
        return "Measures time spent to append signaled wait on event command to in-order command list.";
    }
};