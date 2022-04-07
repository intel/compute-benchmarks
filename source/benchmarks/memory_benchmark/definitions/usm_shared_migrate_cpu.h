/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmSharedMigrateCpuArguments : TestCaseArgumentContainer {
    BooleanArgument accessAllBytes;
    ByteSizeArgument bufferSize;

    UsmSharedMigrateCpuArguments()
        : accessAllBytes(*this, "accessAllBytes", "Select, whether entire resource or only one byte will be accessed on CPU"),
          bufferSize(*this, "size", "Size of the buffer") {}
};

struct UsmSharedMigrateCpu : TestCase<UsmSharedMigrateCpuArguments> {
    using TestCase<UsmSharedMigrateCpuArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedMigrateCpu";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures time to migrate it from GPU to CPU";
    }
};
