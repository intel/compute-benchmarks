/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

#include <cstdint>
#include <limits>

struct UsmCopyBidirectionalKernelArguments : TestCaseArgumentContainer {
    static constexpr size_t elementSize = 16;
    static constexpr size_t stripeElements = 32;
    static constexpr size_t stripePairSize = 2 * stripeElements * elementSize;

    ByteSizeArgument size;
    BooleanArgument useEvents;

    UsmCopyBidirectionalKernelArguments()
        : size(*this, "size", "Size of each buffer. Must be a multiple of 1024 bytes (two stripes)"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        return this->size > 0 && this->size % stripePairSize == 0 && this->size / elementSize <= std::numeric_limits<uint32_t>::max();
    }
};

struct UsmCopyBidirectionalKernel : TestCase<UsmCopyBidirectionalKernelArguments> {
    using TestCase<UsmCopyBidirectionalKernelArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "UsmCopyBidirectionalKernel";
    }

    std::string getHelp() const override {
        return "allocates a unified shared memory buffer in host memory and one in device memory, both of the same size, and measures "
               "the bandwidth of a single kernel that copies in both directions at once. One work item copies 16 bytes and the copy "
               "direction alternates every 512 bytes: even stripes go from host to device, odd stripes from device to host. "
               "The result is the aggregate of both directions, each direction carries half of it.";
    }
};
