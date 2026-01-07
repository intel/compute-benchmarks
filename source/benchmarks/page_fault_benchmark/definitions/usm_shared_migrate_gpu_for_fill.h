/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memadvise_preferred_location_argument.h"
#include "framework/test_case/test_case.h"

struct UsmSharedMigrateGpuForFillArguments : TestCaseArgumentContainer {
    ByteSizeArgument bufferSize;
    BooleanArgument prefetchMemory;
    UsmMemAdvisePreferredLocationArgument preferredLocation;
    BooleanArgument forceBlitter;

    UsmSharedMigrateGpuForFillArguments()
        : bufferSize(*this, "size", "Size of the buffer"),
          prefetchMemory(*this, "prefetch", "Explicitly migrate shared allocation to device associated with command queue"),
          preferredLocation(*this, "preferredLocation", "Apply memadvise with preferred device location (system, device, none)"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()) {}
};

struct UsmSharedMigrateGpuForFill : TestCase<UsmSharedMigrateGpuForFillArguments> {
    using TestCase<UsmSharedMigrateGpuForFillArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedMigrateGpuForFill";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures bandwidth for memory fill operation that must migrate resource from CPU to GPU";
    }
};
