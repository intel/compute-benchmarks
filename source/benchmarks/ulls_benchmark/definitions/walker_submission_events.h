/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct WalkerSubmissionEventsArguments : TestCaseArgumentContainer {};

struct WalkerSubmissionEvents : TestCase<WalkerSubmissionEventsArguments> {
    using TestCase<WalkerSubmissionEventsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "WalkerSubmissionEvents";
    }

    std::string getHelp() const override {
        return "enqueues an empty kernel with GPU-side profiling and checks delta between queue "
               "time and start time.";
    }
};
