/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/enum/atomic_memory_order.h"
#include "framework/enum/atomic_scope.h"
#include "framework/enum/data_type.h"
#include "framework/enum/math_operation.h"

#include <cstddef>

class CompilerOptionsBuilder;
struct MathOperationTestData;

struct KernelHelper {
    static MathOperationTestData getDataForKernel(DataType dataType, MathOperation operation, size_t totalThreadsCount);
    static std::string getCompilerOptions(DataType dataType, MathOperation operation, size_t otherArgumentBufferSize);
    static std::string getCompilerOptionsExplicit(DataType dataType, MathOperation operation, AtomicMemoryOrder order,
                                                  AtomicScope scope, size_t otherArgumentBufferSize);

  private:
    static void addAtomicOpMacro(CompilerOptionsBuilder &options, MathOperation operation);
    static void addExplicitAtomicOpMacro(CompilerOptionsBuilder &options, MathOperation operation, AtomicMemoryOrder order, AtomicScope scope);
};
