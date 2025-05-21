/*
 * Copyright (C) 2022-2025 Intel Corporation
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

struct UsmCopyImmediateArguments : TestCaseArgumentContainer {
    MultiDeviceSelectionArgument contextPlacement;
    DeviceSelectionArgument queuePlacement;
    UsmDeviceSelectionArgument srcPlacement;
    UsmDeviceSelectionArgument dstPlacement;
    ByteSizeArgument size;
    BooleanArgument forceBlitter;
    BooleanArgument useEvents;
    BooleanArgument withCopyOffload;

    UsmCopyImmediateArguments()
        : contextPlacement(*this, "context", "How context will be created"),
          queuePlacement(*this, "queue", "Which device within the context will perform the operation"),
          srcPlacement(*this, "src", "Placement of memory for the source buffer"),
          dstPlacement(*this, "dst", "Placement of memory for the destination buffer"),
          size(*this, "size", "Size of the buffers"),
          forceBlitter(*this, "forceBlitter", CommonHelpMessage::forceBlitter()),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)") {}

    bool validateArgumentsExtra() const override {
        return DeviceSelectionHelper::isSubset(contextPlacement, queuePlacement) &&
               DeviceSelectionHelper::isSubset(contextPlacement, DeviceSelectionHelper::withoutHost(srcPlacement)) &&
               DeviceSelectionHelper::isSubset(contextPlacement, DeviceSelectionHelper::withoutHost(dstPlacement));
    }
};

struct UsmCopyImmediate : TestCase<UsmCopyImmediateArguments> {
    using TestCase<UsmCopyImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyImmediate";
    }

    std::string getHelp() const override {
        return "allocates two unified shared memory buffers and measures copy bandwidth between "
               "them using a builtin function appended to an immediate list.";
    }
};
