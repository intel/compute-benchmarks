/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct WalkerCompletionLatencyArguments : TestCaseArgumentContainer {
    BooleanArgument useFence;
    BooleanArgument inOrderQueue;
    WalkerCompletionLatencyArguments()
        : useFence(*this, "useFence", "Use fence during submission and for further completion."),
          inOrderQueue(*this, "inOrderQueue", "If set use IOQ, otherwise OOQ. Applicable only for OCL.") {
    }
};

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
