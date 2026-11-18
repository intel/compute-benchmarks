/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct UsmConcurrentCopyGraphArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    BooleanArgument withCopyOffload;

    UsmConcurrentCopyGraphArguments()
        : size(*this, "size", "Size of the buffer"),
          withCopyOffload(*this, "withCopyOffload", "Enable driver copy offload on the command lists used for graph capture (only valid for L0)") {}
};

struct UsmConcurrentCopyGraph : TestCase<UsmConcurrentCopyGraphArguments> {
    using TestCase<UsmConcurrentCopyGraphArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmConcurrentCopyGraph";
    }

    std::string getHelp() const override {
        return "allocates four USM buffers, 2 in device memory and 2 in host memory. Captures a graph in which the host to device "
               "and the device to host copies run on two forks that start from the same event, and measures copy bandwidth "
               "of the graph replay with GPU timestamps.";
    }
};
