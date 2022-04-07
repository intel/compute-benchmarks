/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/compression_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct SlmTrafficArguments : TestCaseArgumentContainer {
    ByteSizeArgument size;
    PositiveIntegerArgument occupancyDivider;
    BooleanArgument writeDirection;

    SlmTrafficArguments()
        : size(*this, "size", "SLM Size"),
          occupancyDivider(*this, "occupancyDiv", "H/W load divider by 8, 4, 2, full occupancy"),
          writeDirection(*this, "direction", "write or read mode") {}
};

struct SlmTraffic : TestCase<SlmTrafficArguments> {
    using TestCase<SlmTrafficArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SLM_DataAccessLatency";
    }

    std::string getHelp() const override {
        return "generates SLM local memory transactions inside thread group to measure latency between reads (uses Intel only private intel_get_cycle_counter() )";
    }
};
