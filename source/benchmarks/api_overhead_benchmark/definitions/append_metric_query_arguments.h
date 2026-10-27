/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case_argument_container.h"

struct AppendMetricQueryArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument appendCount;

    AppendMetricQueryArguments()
        : appendCount(*this, "appendCount", "Number of appends to run, each with its own metric query") {}
};
