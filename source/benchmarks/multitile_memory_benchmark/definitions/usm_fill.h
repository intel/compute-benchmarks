/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/argument/enum/device_selection_argument.h"
#include "framework/argument/enum/multi_device_selection_argument.h"
#include "framework/argument/enum/usm_device_selection_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmFillArguments : TestCaseArgumentContainer {
    MultiDeviceSelectionArgument contextPlacement;
    DeviceSelectionArgument queuePlacement;
    UsmDeviceSelectionArgument bufferPlacement;
    ByteSizeArgument size;
    ByteSizeArgument patternSize;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;

    UsmFillArguments()
        : contextPlacement(*this, "context", "How context will be created"),
          queuePlacement(*this, "queue", "Which device within the context will perform the operation"),
          bufferPlacement(*this, "memory", "Placement of memory for the buffer"),
          size(*this, "size", "Size of the buffer"),
          patternSize(*this, "patternSize", "Size of the fill pattern"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        return DeviceSelectionHelper::isSubset(contextPlacement, queuePlacement) &&
               DeviceSelectionHelper::isSubset(contextPlacement, DeviceSelectionHelper::withoutHost(bufferPlacement));
    }
};

struct UsmFill : TestCase<UsmFillArguments> {
    using TestCase<UsmFillArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmFill";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer and measures fill bandwidth.";
    }
};
