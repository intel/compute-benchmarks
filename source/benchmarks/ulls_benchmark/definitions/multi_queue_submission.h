/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/test_case/test_case.h"

struct MultiQueueSubmissionArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument queueCount;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    MultiQueueSubmissionArguments()
        : queueCount(*this, "queueCount", "Number of command queues created"),
          workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size") {}
};

struct MultiQueueSubmission : TestCase<MultiQueueSubmissionArguments> {
    using TestCase<MultiQueueSubmissionArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "MultiQueueSubmission";
    }

    std::string getHelp() const override {
        return "enqueues kernel on multiple command queues";
    }
};
