/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/usm_memory_placement_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct BarrierBetweenKernelsArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument bytesToFlush;
    IntegerArgument onlyReads;
    IntegerArgument remoteAccess;
    UsmMemoryPlacementArgument flushedMemory;

    BarrierBetweenKernelsArguments()
        : bytesToFlush(*this, "bytes", "bytes to flush from L3"),
          onlyReads(*this, "onlyReads", "only reads cached in L3"),
          remoteAccess(*this, "remoteAccess", "access cached from remote tile"),
          flushedMemory(*this, "memoryType", "memory type cached in L3") {}
};

struct BarrierBetweenKernels : TestCase<BarrierBetweenKernelsArguments> {

    using TestCase<BarrierBetweenKernelsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "BarrierBetweenKernels";
    }

    std::string getHelp() const override {
        return "measures time required to run a barrier command between 2 kernels, including potential cache flush commands";
    }
};
