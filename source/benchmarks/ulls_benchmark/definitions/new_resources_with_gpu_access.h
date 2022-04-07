/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct NewResourcesWithGpuAccessArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;

    NewResourcesWithGpuAccessArguments()
        : size(*this, "size", "Size of the buffer") {}
};

struct NewResourcesWithGpuAccess : TestCase<NewResourcesWithGpuAccessArguments> {
    using TestCase<NewResourcesWithGpuAccessArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "NewResourcesWithGpuAccess";
    }

    std::string getHelp() const override {
        return "enqueues kernel that accesses an entire buffer placed in device memory to measure "
               "resource preparation time. The resource is destroyed and recreated for each "
               "iteration to ensure it is a different memory allocation.";
    };
};
