/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SetKernelArgSvmPointerArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument allocationsCount;
    BooleanArgument reallocate;
    PositiveIntegerArgument allocationSize;

    SetKernelArgSvmPointerArguments()
        : allocationsCount(*this, "allocationsCount", "Number of allocations"),
          reallocate(*this, "reallocate", "Allocations will be freed and allocated again between setKernelArgs"),
          allocationSize(*this, "allocationSize", "Size of svm allocations, in bytes") {}
};

struct SetKernelArgSvmPointer : TestCase<SetKernelArgSvmPointerArguments> {
    using TestCase<SetKernelArgSvmPointerArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SetKernelArgSvmPointer";
    }

    std::string getHelp() const override {
        return "measures time spent in clSetKernelArgSVMPointer on CPU.";
    }
};
