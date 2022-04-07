/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/argument/basic_argument.h"
#include "framework/argument/enum/atomic_math_operation_argument.h"
#include "framework/argument/enum/atomic_memory_order_argument.h"
#include "framework/argument/enum/atomic_scope_argument.h"
#include "framework/argument/enum/data_type_argument.h"
#include "framework/test_case/test_case.h"
#include "framework/utility/common_help_message.h"

struct OneAtomicExplicitArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;
    AtomicMathOperationArgument atomicOperation;
    AtomicScopeArgument scope;
    AtomicMemoryOrderArgument memoryOrder;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    OneAtomicExplicitArguments()
        : dataType(*this, "type", CommonHelpMessage::atomicDataType()),
          atomicOperation(*this, "op", "Atomic operation to perform"),
          scope(*this, "scope", "Memory scope of an atomic operation"),
          memoryOrder(*this, "order", "Memory order of an atomic operation"),
          workgroupCount(*this, "wgc", "Work group count"),
          workgroupSize(*this, "wgs", "Work group size") {}
};

struct OneAtomicExplicit : TestCase<OneAtomicExplicitArguments> {
    using TestCase<OneAtomicExplicitArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "OneAtomicExplicit";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing an atomic operation on a single address using OpenCL 2.0 Atomics with explicit memory order and scope";
    }
};
