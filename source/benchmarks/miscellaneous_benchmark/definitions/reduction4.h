/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct ReductionArguments4 : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfElements;

    ReductionArguments4()
        : numberOfElements(*this, "numberOfElements", "Number of elements that will be reduced") {}
};

struct Reduction4 : TestCase<ReductionArguments4> {
    using TestCase<ReductionArguments4>::TestCase;

    std::string getTestCaseName() const override {
        return "Reduction4";
    }

    std::string getHelp() const override {
        return "Performs a reduction operation on a buffer. Each thread performs atomic_add on "
               "one shared memory location.";
    }
};
