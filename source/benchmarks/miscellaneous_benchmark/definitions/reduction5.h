/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct ReductionArguments5 : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfElements;

    ReductionArguments5()
        : numberOfElements(*this, "numberOfElements", "Number of elements that will be reduced") {}
};

struct Reduction5 : TestCase<ReductionArguments5> {
    using TestCase<ReductionArguments5>::TestCase;

    std::string getTestCaseName() const override {
        return "Reduction5";
    }

    std::string getHelp() const override {
        return "Performs a reduction operation on a buffer. Each thread performs atomic_add on "
               "one shared memory location.";
    }
};
