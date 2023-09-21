/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct QueueConcurrencyArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument kernelTime;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument kernelCount;

    QueueConcurrencyArguments()
        : kernelTime(*this, "kernelTime", "How long each work item is in kernel"),
          workgroupCount(*this, "workgroupCount", "Workgroup Count of each kernel"),
          kernelCount(*this, "kernelCount", "How many kernels are submitted") {}
};

struct QueueConcurrency : TestCase<QueueConcurrencyArguments> {
    using TestCase<QueueConcurrencyArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "QueueConcurrency";
    }

    std::string getHelp() const override {
        return "Submits multiple kernels to out of order queue returning events. Then calls synchronization and meassures performance";
    }
};
