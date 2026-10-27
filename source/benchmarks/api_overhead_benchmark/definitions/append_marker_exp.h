/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct AppendMarkerExpArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument appendCount;

    AppendMarkerExpArguments()
        : appendCount(*this, "appendCount", "Number of appends to run") {}
};

struct AppendMarkerExp : TestCase<AppendMarkerExpArguments> {
    using TestCase<AppendMarkerExpArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "AppendMarkerExp";
    }

    std::string getHelp() const override {
        return "measures time spent in zetCommandListAppendMarkerExp on CPU. "
               "The first metric group of the device that can generate markers is activated. "
               "Requires ZET_ENABLE_METRICS=1.";
    }
};
