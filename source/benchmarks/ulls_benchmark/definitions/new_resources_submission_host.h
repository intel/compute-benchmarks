/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct NewResourcesSubmissionHostArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;

    NewResourcesSubmissionHostArguments()
        : size(*this, "size", "Size of the buffer") {}
};

struct NewResourcesSubmissionHost : TestCase<NewResourcesSubmissionHostArguments> {
    using TestCase<NewResourcesSubmissionHostArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "NewResourcesSubmissionHost";
    }

    std::string getHelp() const override {
        return "enqueues kernel that uses a buffer placed in host memory to measure resource "
               "preparation time. The resource is destroyed and recreated for each iteration "
               "to ensure it is a different memory allocation.";
    };
};
