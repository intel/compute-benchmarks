/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct ReductionArguments3 : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfElements;

    ReductionArguments3()
        : numberOfElements(*this, "numberOfElements", "Number of elements that will be reduced") {}
};

struct Reduction3 : TestCase<ReductionArguments3> {
    using TestCase<ReductionArguments3>::TestCase;

    std::string getTestCaseName() const override {
        return "Reduction3";
    }

    std::string getHelp() const override {
        return "Performs a reduction operation on a buffer. Each thread performs atomic_add on "
               "one shared memory location.";
    }
};
