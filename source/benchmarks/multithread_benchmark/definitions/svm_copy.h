/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct SvmCopyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument numberOfThreads;

    SvmCopyArguments()
        : numberOfThreads(*this, "numberOfThreads", "Number of threads that will run concurrently") {}
};

struct SvmCopy : TestCase<SvmCopyArguments> {
    using TestCase<SvmCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SvmCopy";
    }

    std::string getHelp() const override {
        return "enqueues multiple svm copies on multiple threads concurrently.";
    }
};
