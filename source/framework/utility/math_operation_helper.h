/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/data_type.h"
#include "framework/enum/math_operation.h"

struct MathOperationTestData {
    size_t sizeOfDataType;
    size_t loopIterations;
    std::byte initialValue[8];
    std::byte otherArgument[8];
    std::byte expectedValue[8];
};

struct MathOperationHelper {
    static bool requiresIntelGlobalAtomicsExtension(MathOperation operation, DataType type);
    static bool isSupportedAsAtomic(MathOperation operation, DataType type, bool globalAtomicFloatsSupported, bool usesSlm);
    static bool isSupportedAsNormal(MathOperation operation, DataType type);
    static size_t getArgumentsCount(MathOperation operation);

    static MathOperationTestData generateTestData(DataType dataType, MathOperation operation, size_t loopIterations,
                                                  size_t operationsPerLoop, size_t totalThreadsCount = 1);
    template <typename DataTypeT>
    static MathOperationTestData generateTestData(MathOperation operation, size_t loopIterations, size_t operationsPerLoop, size_t totalThreadsCount = 1);
};

template <typename DataTypeT>
inline MathOperationTestData MathOperationHelper::generateTestData(MathOperation operation, size_t loopIterations, size_t operationsPerLoop, size_t totalThreadsCount) {
    const size_t operationsCount = totalThreadsCount * loopIterations * operationsPerLoop;

    MathOperationTestData result = {};
    result.sizeOfDataType = sizeof(DataTypeT);
    result.loopIterations = loopIterations;
    DataTypeT &initialValue = reinterpret_cast<DataTypeT &>(result.initialValue);
    DataTypeT &otherArgument = reinterpret_cast<DataTypeT &>(result.otherArgument);
    DataTypeT &expectedValue = reinterpret_cast<DataTypeT &>(result.expectedValue);

    switch (operation) {
    case MathOperation::Add:
        initialValue = 1000;
        otherArgument = 3;
        expectedValue = initialValue + otherArgument * static_cast<DataTypeT>(operationsCount);
        break;
    case MathOperation::Sub:
        initialValue = 1000;
        otherArgument = 3;
        expectedValue = initialValue - otherArgument * static_cast<DataTypeT>(operationsCount);
        break;
    case MathOperation::Div:
        initialValue = std::numeric_limits<DataTypeT>::max() - 1;
        otherArgument = 2;
        expectedValue = 0; // technically only for big enough numbers
        break;
    case MathOperation::Xchg:
        initialValue = 7;
        otherArgument = 3;
        expectedValue = otherArgument;
        break;
    case MathOperation::CmpXchg:
        initialValue = 7;
        otherArgument = 3;
        expectedValue = initialValue;
        break;
    case MathOperation::Inc:
        initialValue = 12;
        otherArgument = 0;
        expectedValue = initialValue + static_cast<DataTypeT>(operationsCount);
        break;
    case MathOperation::Dec:
        initialValue = 43;
        otherArgument = 0;
        expectedValue = initialValue - static_cast<DataTypeT>(operationsCount);
        break;
    case MathOperation::Min:
        initialValue = 100;
        otherArgument = 33;
        expectedValue = std::min(initialValue, otherArgument);
        break;
    case MathOperation::Max:
        initialValue = 100;
        otherArgument = 33;
        expectedValue = std::max(initialValue, otherArgument);
        break;
    default:
        if constexpr (std::is_integral_v<DataTypeT>) {
            switch (operation) {
            case MathOperation::Modulo:
                initialValue = 123;
                otherArgument = 100;
                expectedValue = 23;
                break;
            case MathOperation::And:
                initialValue = 0b1111001111001101;
                otherArgument = 0b1101100100101010;
                expectedValue = initialValue & otherArgument;
                break;
            case MathOperation::Or:
                initialValue = 0b1111001111001101;
                otherArgument = 0b1101100100101010;
                expectedValue = initialValue | otherArgument;
                break;
            case MathOperation::Xor:
                initialValue = 0b1111001111001101;
                otherArgument = 0b1101100100101010;
                expectedValue = (operationsCount % 2 == 0) ? initialValue : initialValue ^ otherArgument;
                break;
            default:
                FATAL_ERROR("Invalid math operation");
            }
        } else {
            FATAL_ERROR("Invalid math operation");
        }
    }
    return result;
}
