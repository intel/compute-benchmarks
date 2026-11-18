/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/engine_argument.h"
#include "framework/test_case/test_case.h"

struct UsmConcurrentCopyImmediateArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    EngineArgument h2dEngine;
    EngineArgument d2hEngine;
    BooleanArgument withCopyOffload;

    UsmConcurrentCopyImmediateArguments()
        : size(*this, "size", "Size of the buffer"),
          h2dEngine(*this, "h2dEngine", "Engine used for host to device copy"),
          d2hEngine(*this, "d2hEngine", "Engine used for device to host copy"),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload (only valid for L0)") {}
};

struct UsmConcurrentCopyImmediate : TestCase<UsmConcurrentCopyImmediateArguments> {
    using TestCase<UsmConcurrentCopyImmediateArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmConcurrentCopyImmediate";
    }

    std::string getHelp() const override {
        return "allocates four USM buffers, 2 in device memory and 2 in host memory. Measures concurrent copy bandwidth between them using immediate command lists.";
    }
};
