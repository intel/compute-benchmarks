/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct CommandListQueryArguments : TestCaseArgumentContainer {
    BooleanArgument useImmediate;

    CommandListQueryArguments()
        : useImmediate(*this, "useImmediate", "Query an immediate command list instead of a regular one") {}
};
