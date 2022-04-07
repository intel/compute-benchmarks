/*
 * Copyright (C) 2022 Intel Corporation
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

struct OneLocalAtomicArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;
    AtomicMathOperationArgument atomicOperation;
    PositiveIntegerArgument workgroupSize;

    OneLocalAtomicArguments()
        : dataType(*this, "type", CommonHelpMessage::atomicDataType()),
          atomicOperation(*this, "op", "Atomic operation to perform"),
          workgroupSize(*this, "wgs", "Work group size") {}
};

struct OneLocalAtomic : TestCase<OneLocalAtomicArguments> {
    using TestCase<OneLocalAtomicArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "OneLocalAtomic";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing an atomic operation on a single location placed in SLM";
    }
};
