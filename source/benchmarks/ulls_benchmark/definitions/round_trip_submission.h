/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct RoundTripSubmissionArguments : TestCaseArgumentContainer {};

struct RoundTripSubmission : TestCase<RoundTripSubmissionArguments> {
    using TestCase<RoundTripSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "RoundTripSubmission";
    }

    std::string getHelp() const override {
        return "enqueues kernel which updates system memory location and waits for it with a synchronizing API.";
    }
};
