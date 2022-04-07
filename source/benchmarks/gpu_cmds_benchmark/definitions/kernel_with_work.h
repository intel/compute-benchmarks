/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/work_item_id_usage_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct KernelWithWorkArguments : TestCaseArgumentContainer {
    WorkItemIdUsageArgument usedIds;
    PositiveIntegerArgument measuredCommands;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    KernelWithWorkArguments()
        : usedIds(*this, "usedIds", "Which of the get_global_id() and get_local_id() calls will be used in the kernel"),
          measuredCommands(*this, "measuredCommands", CommonHelpMessage::measuredCommandsCount()),
          workgroupCount(*this, "wgc", "Workgroup count"),
          workgroupSize(*this, "wgs", "Workgroup size (aka local work size)") {}
};

struct KernelWithWork : TestCase<KernelWithWorkArguments> {

    using TestCase<KernelWithWorkArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "KernelWithWork";
    }

    std::string getHelp() const override {
        return "measures time required to run a GPU kernel which assigns values to elements of a buffer.";
    }
};
