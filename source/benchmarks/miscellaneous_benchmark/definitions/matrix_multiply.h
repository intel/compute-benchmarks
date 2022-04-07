/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

struct MatrixMultiplyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfElementsX;
    PositiveIntegerArgument numberOfElementsY;
    PositiveIntegerArgument numberOfElementsZ;

    MatrixMultiplyArguments()
        : numberOfElementsX(*this, "numberOfElementsX", "Number of elements in X dimension"),
          numberOfElementsY(*this, "numberOfElementsY", "Number of elements in Y dimension"),
          numberOfElementsZ(*this, "numberOfElementsZ", "Number of elements in Z dimension") {}
};

struct MatrixMultiply : TestCase<MatrixMultiplyArguments> {
    using TestCase<MatrixMultiplyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "VectorSum";
    }

    std::string getHelp() const override {
        return "Performs vector addition";
    }
};
