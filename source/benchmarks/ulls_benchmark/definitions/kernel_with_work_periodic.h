/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"

#include <sstream>

struct KernelWithWorkPeriodicArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument timeBetweenSubmissions;
    PositiveIntegerArgument numSubmissions;

    KernelWithWorkPeriodicArguments()
        : timeBetweenSubmissions(*this, "timeBetweenSubmissions", "Delay between kernel enqueues in microseconds"),
          numSubmissions(*this, "numSubmissions", "Number of kernel enqueues to run") {}
};

struct KernelWithWorkPeriodic : TestCase<KernelWithWorkPeriodicArguments> {
    using TestCase<KernelWithWorkPeriodicArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelWithWorkPeriodic";
    }

    std::string getHelp() const override {
        return "measures average time required to run a GPU kernel which assigns constant values to "
               "elements of a buffer. Each thread assigns one value. "
               "Kernel is run multiple times with a set delay between submissions.";
    }
};
