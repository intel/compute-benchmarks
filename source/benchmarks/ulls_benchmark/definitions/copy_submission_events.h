/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"

struct CopySubmissionEventsArguments : TestCaseArgumentContainer {
    EngineArgument engine;

    CopySubmissionEventsArguments() : engine(*this, "engine", "Engine used for copying") {}
};

struct CopySubmissionEvents : TestCase<CopySubmissionEventsArguments> {
    using TestCase<CopySubmissionEventsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "CopySubmissionEvents";
    }

    std::string getHelp() const override {
        return "enqueues 4 byte copy to copy engine and return submission delta "
               "which is time between host API call and copy engine start";
    }
};
