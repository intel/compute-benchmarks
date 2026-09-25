/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/bitmap_argument.h"
#include "framework/enum/engine.h"
#include "framework/test_case/test_case.h"

using CcsBitmaskArgument = BitmaskArgument<EngineHelper::maxNumberOfComputeEngines, true>;
using BcsBitmaskArgument = BitmaskArgument<EngineHelper::maxNumberOfCopyEngines, true>;

struct ZMultiEngineSubmissionEventsArguments : TestCaseArgumentContainer {
    CcsBitmaskArgument ccsMask;
    BcsBitmaskArgument bcsMask;
    BooleanArgument useHpCopyEngine;

    ZMultiEngineSubmissionEventsArguments()
        : ccsMask(*this, "ccsMask", "A bit mask for selecting compute engines, rightmost bit is CCS0"),
          bcsMask(*this, "bcsMask", "A bit mask for selecting copy engines, rightmost bit is the first copy engine reported by the driver"),
          useHpCopyEngine(*this, "useHpCopyEngine", "Adds one more list for the first copy engine reported by the driver, created with high priority") {}

    bool validateArgumentsExtra() const override {
        return CcsBitmaskArgument::Bitset(ccsMask).any() || BcsBitmaskArgument::Bitset(bcsMask).any() || useHpCopyEngine;
    }
};

struct ZMultiEngineSubmissionEvents : TestCase<ZMultiEngineSubmissionEventsArguments> {
    using TestCase<ZMultiEngineSubmissionEventsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "ZMultiEngineSubmissionEvents";
    }

    std::string getHelp() const override {
        return "creates one immediate command list for each engine from ccsMask and "
               "bcsMask, submits one command to all lists, and measures the submit "
               "delta. The delta is the time from the host API call to engine start. "
               "The test reports the delta for each engine and the worst value. If "
               "useHpCopyEngine is set, the test adds one high-priority copy engine "
               "list.";
    }
};
