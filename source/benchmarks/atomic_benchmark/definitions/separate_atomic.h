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
#include "framework/utility/memory_constants.h"

struct SeparateAtomicsArguments : TestCaseArgumentContainer {
    DataTypeArgument dataType;
    AtomicMathOperationArgument atomicOperation;
    PositiveIntegerArgument atomicsPerCacheline;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    SeparateAtomicsArguments()
        : dataType(*this, "type", CommonHelpMessage::atomicDataType()),
          atomicOperation(*this, "op", "Atomic operation to perform"),
          atomicsPerCacheline(*this, "atomicsPerCacheline", "Number of used addresses occupying a single cacheline (this causes operations to be serialized)"),
          workgroupCount(*this, "wgc", "Work group count"),
          workgroupSize(*this, "wgs", "Work group size") {}

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

struct SeparateAtomics : TestCase<SeparateAtomicsArguments> {
    using TestCase<SeparateAtomicsArguments>::TestCase;

    std::string getTestCaseName() const override {
        return "SeparateAtomics";
    }

    std::string getHelp() const override {
        return "enqueues kernel performing an atomic operation on different addresses";
    }
};
