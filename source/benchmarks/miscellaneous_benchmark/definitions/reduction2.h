/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct ReductionArguments2 : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfElements;

    ReductionArguments2()
        : numberOfElements(*this, "numberOfElements", "Number of elements that will be reduced") {}
};

struct Reduction2 : TestCase<ReductionArguments2> {
    using TestCase<ReductionArguments2>::TestCase;

    std::string getTestCaseName() const override {
        return "Reduction2";
    }

    std::string getHelp() const override {
        return "Performs a reduction operation on a buffer. Each thread performs atomic_add on "
               "one shared memory location.";
    }
};
