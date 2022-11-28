/*
 * Copyright (C) 2022-2023 Intel Corporation
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
#include "framework/utility/memory_constants.h"

struct SeparateAtomicsExplicitArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;
    AtomicMathOperationArgument atomicOperation;
    PositiveIntegerArgument atomicsPerCacheline;
    AtomicScopeArgument scope;
    AtomicMemoryOrderArgument memoryOrder;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;
    BooleanArgument useEvents;

    SeparateAtomicsExplicitArguments()
        : dataType(*this, "type", CommonHelpMessage::atomicDataType()),
          atomicOperation(*this, "op", "Atomic operation to perform"),
          atomicsPerCacheline(*this, "atomicsPerCacheline", "Number of used addresses occupying a single cacheline (this causes operations to be serialized)"),
          scope(*this, "scope", "Memory scope of an atomic operation"),
          memoryOrder(*this, "order", "Memory order of an atomic operation"),
          workgroupCount(*this, "wgc", "Work group count"),
          workgroupSize(*this, "wgs", "Work group size"),
          useEvents(*this, "useEvents", CommonHelpMessage::useEvents()) {}

    bool validateArgumentsExtra() const override {
        if (atomicsPerCacheline > workgroupCount * workgroupSize) {
            return false;
        }
        if (atomicsPerCacheline > MemoryConstants::cachelineSize / DataTypeHelper::getSize(dataType)) {
            return false;
        }
        return true;
    }
};

struct SeparateAtomicsExplicit : TestCase<SeparateAtomicsExplicitArguments> {
    using TestCase<SeparateAtomicsExplicitArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SeparateAtomicsExplicit";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing an atomic operation on different addresses";
    }
};
