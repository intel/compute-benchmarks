/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/atomic_math_operation_argument.h"
#include "framework/argument/enum/data_type_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct OneAtomicArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;
    AtomicMathOperationArgument atomicOperation;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanArgument useEvents;

    OneAtomicArguments()
        : dataType(*this, "type", CommonHelpMessage::atomicDataType()),
          atomicOperation(*this, "op", "Atomic operation to perform"),
          workgroupCount(*this, "wgc", "Work group count"),
          workgroupSize(*this, "wgs", "Work group size"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}
};

struct OneAtomic : TestCase<OneAtomicArguments> {
    using TestCase<OneAtomicArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "OneAtomic";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing an atomic operation on a single address";
    }
};
