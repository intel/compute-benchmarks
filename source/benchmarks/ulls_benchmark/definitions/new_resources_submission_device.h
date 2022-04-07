/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct NewResourcesSubmissionDeviceArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;

    NewResourcesSubmissionDeviceArguments()
        : size(*this, "size", "Size of the buffer") {}
};

struct NewResourcesSubmissionDevice : TestCase<NewResourcesSubmissionDeviceArguments> {
    using TestCase<NewResourcesSubmissionDeviceArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "NewResourcesSubmissionDevice";
    }

    std::string getHelp() const override {
        return "enqueues kernel that uses a buffer placed in device memory to measure resource "
               "preparation time. The resource is destroyed and recreated for each iteration "
               "to ensure it is a different memory allocation.";
    };
};
