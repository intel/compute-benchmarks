/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/enum/stream_memory_type_argument.h"
#include "framework/test_case/test_case.h"

struct FullRemoteAccessMemoryXeCoresDistributedArguments : TestCaseArgumentContainer {
    StreamMemoryTypeArgument type;
    ByteSizeArgument size;
    PositiveIntegerArgument elementSize;
    BooleanArgument useEvents;
    PositiveIntegerArgument workItems;
    BooleanArgument blockAccess;

    FullRemoteAccessMemoryXeCoresDistributedArguments()
        : type(*this, "type", "Memory streaming type"),
          size(*this, "size", "Size of the memory to stream. Must be a power of 2"),
          elementSize(*this, "elementSize", "Size of the single element to read in bytes (1, 2, 4, 8)"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          workItems(*this, "workItems", "Number of work items equal to SIMD size * used hwthreads"),
          blockAccess(*this, "blockAccess", "Block access (1) or scatter access (0)") {}
};

struct FullRemoteAccessMemoryXeCoresDistributed : TestCase<FullRemoteAccessMemoryXeCoresDistributedArguments> {
    using TestCase<FullRemoteAccessMemoryXeCoresDistributedArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "FullRemoteAccessMemoryXeCoresDistributed";
    }

    std::string getHelp() const override {
        return "Uses stream memory in a fashion described by 'type' to measure bandwidth of full remote memory access"
               "when hwthreads are distributed between XeCores.";
    }
};
