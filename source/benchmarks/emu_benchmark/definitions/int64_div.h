/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/data_type_argument.h"
#include "framework/argument/enum/normal_math_operation_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct Int64DivArguments : TestCaseArgumentContainer {
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanArgument useEvents;

    Int64DivArguments()
        : workgroupCount(*this, "wgc", "Work group count"),
          workgroupSize(*this, "wgs", "Work group size"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct Int64Div : TestCase<Int64DivArguments> {
    using TestCase<Int64DivArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "Int64Div";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing an int64 division emulation";
    }
};
