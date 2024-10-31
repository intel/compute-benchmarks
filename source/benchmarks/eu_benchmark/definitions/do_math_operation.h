/*
 * Copyright (C) 2022-2024 Intel Corporation
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

struct DoMathOperationArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;
    NormalMathOperationArgument operation;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanArgument useEvents;
    BooleanArgument mixGrfModes;

    DoMathOperationArguments()
        : dataType(*this, "type", CommonHelpMessage::atomicDataType()),
          operation(*this, "op", "Math operation to perform"),
          workgroupCount(*this, "wgc", "Work group count"),
          workgroupSize(*this, "wgs", "Work group size"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()),
          mixGrfModes(*this, "mixGrf", "Run kernels with mixed grf modes") {}
};

struct DoMathOperation : TestCase<DoMathOperationArguments> {
    using TestCase<DoMathOperationArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "DoMathOperation";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing a math operation";
    }
};
