/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct ReductionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfElements;

    ReductionArguments()
        : numberOfElements(*this, "numberOfElements", "Number of elements that will be reduced") {}
};

struct Reduction : TestCase<ReductionArguments> {
    using TestCase<ReductionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "Reduction";
    }

    std::string getHelp() const override {
        return "Performs a reduction operation on a buffer. Each thread performs atomic_add on "
               "one shared memory location.";
    }
};
