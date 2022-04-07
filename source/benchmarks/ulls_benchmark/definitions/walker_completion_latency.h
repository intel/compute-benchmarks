/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct WalkerCompletionLatencyArguments : TestCaseArgumentContainer {};

struct WalkerCompletionLatency : TestCase<WalkerCompletionLatencyArguments> {
    using TestCase<WalkerCompletionLatencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WalkerCompletionLatency";
    }

    std::string getHelp() const override {
        return "enqueues a kernel writing to system memory and measures time between the moment when "
               "update is visible on CPU and the moment when synchronizing call returns";
    }
};
