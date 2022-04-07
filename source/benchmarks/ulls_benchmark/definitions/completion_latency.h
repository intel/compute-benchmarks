/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct CompletionLatencyArguments : TestCaseArgumentContainer {};

struct CompletionLatency : TestCase<CompletionLatencyArguments> {
    using TestCase<CompletionLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CompletionLatency";
    }

    std::string getHelp() const override {
        return "enqueues system memory write and measures time between the moment, when update is "
               "visible on CPU and the moment, when synchronizing call returns.";
    }
};
