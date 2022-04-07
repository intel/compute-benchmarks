/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_initial_placement_argument.h"
#include "framework/test_case/test_case.h"

struct UsmSharedFirstGpuAccessArguments : TestCaseArgumentContainer {
    UsmInitialPlacementArgument initialPlacement;
    ByteSizeArgument bufferSize;

    UsmSharedFirstGpuAccessArguments()
        : initialPlacement(*this, "initialPlacement", "Hint for initial placement of the resource passed to the driver"),
          bufferSize(*this, "size", "Size of the buffer") {}
};

struct UsmSharedFirstGpuAccess : TestCase<UsmSharedFirstGpuAccessArguments> {
    using TestCase<UsmSharedFirstGpuAccessArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedFirstGpuAccess";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures time to access it on GPU after creation.";
    }
};
