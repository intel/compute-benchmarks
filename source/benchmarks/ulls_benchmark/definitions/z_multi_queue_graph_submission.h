/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/engine_bitmask_argument.h"
#include "framework/test_case/test_case.h"

struct ZMultiQueueGraphSubmissionArguments : TestCaseArgumentContainer {
    CcsBitmaskArgument ccsMask;
    BcsBitmaskArgument bcsMask;
    BooleanArgument useHpCopyEngine;

    ZMultiQueueGraphSubmissionArguments()
        : ccsMask(*this, "ccsMask", "A bit mask for selecting compute engines the graph forks to, rightmost bit is CCS0. Engines that the device does not have are skipped"),
          bcsMask(*this, "bcsMask", "A bit mask for selecting copy engines the graph forks to, rightmost bit is the first copy engine reported by the driver. Engines that the device does not have are skipped"),
          useHpCopyEngine(*this, "useHpCopyEngine", "Adds one more branch for the first copy engine reported by the driver, on a list created with high priority") {}

    bool validateArgumentsExtra() const override {
        return CcsBitmaskArgument::Bitset(this->ccsMask).any() || BcsBitmaskArgument::Bitset(this->bcsMask).any() || this->useHpCopyEngine;
    }
};

struct ZMultiQueueGraphSubmission : TestCase<ZMultiQueueGraphSubmissionArguments> {
    using TestCase<ZMultiQueueGraphSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ZMultiQueueGraphSubmission";
    }

    std::string getHelp() const override {
        return "replays a graph that forks from CCS0 to the engines from ccsMask and bcsMask, and "
               "measures the start latency of each branch and the CPU time of the replay call.";
    }
};
