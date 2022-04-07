/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/multi_device_selection_argument.h"
#include "framework/argument/enum/usm_device_selection_argument.h"
#include "framework/test_case/test_case.h"

struct UsmSharedMigrateCpuArguments : TestCaseArgumentContainer {
    MultiDeviceSelectionArgument contextPlacement;
    UsmSharedDeviceSelectionArgument bufferPlacement;
    ByteSizeArgument bufferSize;
    BooleanArgument accessAllBytes;

    UsmSharedMigrateCpuArguments()
        : contextPlacement(*this, "context", "How context will be created"),
          bufferPlacement(*this, "memory", "Placement of memory for the buffer"),
          bufferSize(*this, "size", "Size of the buffer"),
          accessAllBytes(*this, "accessAllBytes", "Select, whether entire resource or only one byte will be accessed on CPU") {}

    bool validateArgumentsExtra() const override {
        return DeviceSelectionHelper::isSubset(contextPlacement, DeviceSelectionHelper::withoutHost(bufferPlacement));
    }
};

struct UsmSharedMigrateCpu : TestCase<UsmSharedMigrateCpuArguments> {
    using TestCase<UsmSharedMigrateCpuArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedMigrateCpu";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures time to migrate it from GPU to CPU.";
    }
};
