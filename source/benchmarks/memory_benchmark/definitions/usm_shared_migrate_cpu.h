/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memadvise_preferred_location_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmSharedMigrateCpuArguments : TestCaseArgumentContainer {
    BooleanArgument accessAllBytes;
    ByteSizeArgument bufferSize;
    UsmMemAdvisePreferredLocationArgument preferredLocation;

    UsmSharedMigrateCpuArguments()
        : accessAllBytes(*this, "accessAllBytes", "Select, whether entire resource or only one byte will be accessed on CPU"),
          bufferSize(*this, "size", "Size of the buffer"),
          preferredLocation(*this, "preferredLocation", "Apply memadvise with preferred device location (system, device, none)") {}
};

struct UsmSharedMigrateCpu : TestCase<UsmSharedMigrateCpuArguments> {
    using TestCase<UsmSharedMigrateCpuArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedMigrateCpu";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures bandwidth for kernel that must migrate resource from GPU to CPU";
    }
};
