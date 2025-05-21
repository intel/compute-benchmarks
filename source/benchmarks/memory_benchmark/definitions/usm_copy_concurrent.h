/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct UsmConcurrentCopyArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    EngineArgument h2dEngine;
    EngineArgument d2hEngine;
    BooleanArgument withCopyOffload;

    UsmConcurrentCopyArguments()
        : size(*this, "size", "Size of the buffer"),
          h2dEngine(*this, "h2dEngine", "Engine used for host to device copy"),
          d2hEngine(*this, "d2hEngine", "Engine used for device to host copy"),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)") {}
};

struct UsmConcurrentCopy : TestCase<UsmConcurrentCopyArguments> {
    using TestCase<UsmConcurrentCopyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmConcurrentCopy";
    }

    std::string getHelp() const override {
        return "allocates four unified shared memory buffers, 2 in device memory and 2 in host memory. Measures concurrent copy bandwidth between them.";
    }
};
