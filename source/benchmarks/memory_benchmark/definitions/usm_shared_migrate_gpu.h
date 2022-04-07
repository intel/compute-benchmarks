/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct UsmSharedMigrateGpuArguments : TestCaseArgumentContainer {
    ByteSizeArgument bufferSize;

    UsmSharedMigrateGpuArguments()
        : bufferSize(*this, "size", "Size of the buffer") {}
};

struct UsmSharedMigrateGpu : TestCase<UsmSharedMigrateGpuArguments> {
    using TestCase<UsmSharedMigrateGpuArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedMigrateGpu";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures time to migrate it from CPU to GPU";
    }
};
