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

struct UsmSharedMigrateGpuArguments : TestCaseArgumentContainer {
    MultiDeviceSelectionArgument contextPlacement;
    UsmSharedDeviceSelectionArgument bufferPlacement;
    ByteSizeArgument bufferSize;

    UsmSharedMigrateGpuArguments()
        : contextPlacement(*this, "context", "How context will be created"),
          bufferPlacement(*this, "memory", "Placement of memory for the buffer"),
          bufferSize(*this, "size", "Size of the buffer") {}

    bool validateArgumentsExtra() const override {
        return DeviceSelectionHelper::isSubset(contextPlacement, DeviceSelectionHelper::withoutHost(bufferPlacement));
    }
};

struct UsmSharedMigrateGpu : TestCase<UsmSharedMigrateGpuArguments> {
    using TestCase<UsmSharedMigrateGpuArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmSharedMigrateGpu";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures time to migrate it from CPU to GPU.";
    }
};
