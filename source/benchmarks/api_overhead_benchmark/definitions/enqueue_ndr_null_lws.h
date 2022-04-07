/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct EnqueueNdrNullLwsArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument gws;
    BooleanArgument useOoq;
    BooleanArgument useProfiling;
    BooleanArgument useEvent;

    EnqueueNdrNullLwsArguments()
        : gws(*this, "gws", "Global work size"),
          useOoq(*this, "ooq", "Use out of order queue"),
          useProfiling(*this, "profiling", "Creating a profiling queue"),
          useEvent(*this, "event", "Pass output event to the enqueue call") {}
};

struct EnqueueNdrNullLws : TestCase<EnqueueNdrNullLwsArguments> {
    using TestCase<EnqueueNdrNullLwsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "EnqueueNdrNullLws";
    }

    std::string getHelp() const override {
        return "measures time spent in clEnqueueNDRangeKernel on CPU. Null LWS is provided, which "
               "causes driver to calculate it";
    }
};
