/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/data_type.h"
#include "framework/enum/math_operation.h"

#include <cstddef>
#include <string>

struct DoMathOperationArguments;
struct MathOperationTestData;

enum class OperationChain {
    Repeated,
    Coupled,
    Mixed,
};

struct KernelHelper {
    static OperationChain getOperationChain(DataType dataType, MathOperation operation);
    static std::string getCompilerOptions(DataType dataType, MathOperation operation, OperationChain chain);
    static MathOperationTestData getExpectedResult(const DoMathOperationArguments &arguments, OperationChain chain, size_t loopIterations,
                                                   size_t operationsPerLoop, size_t enqueueCount);

  private:
    static MathOperation getMixingOperation(MathOperation operation);
    static std::string getAdditiveExpression(DataType dataType, MathOperation operation, bool wrapping);
    static std::string getOperationExpression(DataType dataType, MathOperation operation, bool wrapping);
    template <typename DataTypeT>
    static DataTypeT applyOperation(MathOperation operation, DataTypeT value, DataTypeT operand);
    template <typename DataTypeT>
    static void computeChainedResult(MathOperation operation, OperationChain chain, size_t enqueueCount, size_t pairsPerEnqueue, MathOperationTestData &data);
};
