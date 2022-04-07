/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_helper.h"

#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/math_operation_helper.h"

#include <algorithm>

MathOperationTestData KernelHelper::getDataForKernel(DataType dataType,
                                                     MathOperation operation,
                                                     size_t totalThreadsCount) {
    const size_t loopIterations = 100u;                   // Kernel performs some number of loop iterations
    const size_t operatorApplicationsPerIteration = 128u; // Each iteration performs some number of atomic operations
    return MathOperationHelper::generateTestData(dataType, operation, loopIterations, operatorApplicationsPerIteration, totalThreadsCount);
}

std::string KernelHelper::getCompilerOptions(DataType dataType, MathOperation operation, size_t otherArgumentBufferSize) {
    CompilerOptionsBuilder options{};
    addAtomicOpMacro(options, operation);
    if (MathOperationHelper::requiresIntelGlobalAtomicsExtension(operation, dataType)) {
        options.addOptionOpenCl20();
        options.addDefinition("cl_intel_global_float_atomics");
        options.addDefinition("USE_GLOBAL_FLOAT_ATOMICS");
    }
    options.addDefinitionKeyValue("ATOMIC_DATATYPE", DataTypeHelper::toOpenclC(dataType));
    options.addDefinitionKeyValue("DATATYPE", DataTypeHelper::toOpenclC(dataType));
    options.addDefinitionKeyValue("OTHER_ARGUMENT_BUFFER_SIZE", std::to_string(otherArgumentBufferSize));
    return options.str();
}

std::string KernelHelper::getCompilerOptionsExplicit(DataType dataType, MathOperation operation, AtomicMemoryOrder order,
                                                     AtomicScope scope, size_t otherArgumentBufferSize) {
    CompilerOptionsBuilder options{};
    addExplicitAtomicOpMacro(options, operation, order, scope);
    options.addOptionOpenCl20();
    options.addDefinition("OCL_20");
    if (MathOperationHelper::requiresIntelGlobalAtomicsExtension(operation, dataType)) {
        options.addDefinition("cl_intel_global_float_atomics");
        options.addDefinition("USE_GLOBAL_FLOAT_ATOMICS");
    }
    options.addDefinitionKeyValue("ATOMIC_DATATYPE", DataTypeHelper::toExplicitAtomicOpenclC(dataType));
    options.addDefinitionKeyValue("DATATYPE", DataTypeHelper::toOpenclC(dataType));
    options.addDefinitionKeyValue("OTHER_ARGUMENT_BUFFER_SIZE", std::to_string(otherArgumentBufferSize));
    return options.str();
}

void KernelHelper::addAtomicOpMacro(CompilerOptionsBuilder &options, MathOperation operation) {
    const static char *functionNames[] = {"ERROR",
                                          "atomic_add", "atomic_sub",
                                          "atomic_xchg", "atomic_cmpxchg",
                                          "atomic_inc", "atomic_dec",
                                          "atomic_min", "atomic_max",
                                          "atomic_and", "atomic_or",
                                          "atomic_xor"};

    std::ostringstream macroBody{};
    macroBody << functionNames[static_cast<int>(operation)] << "(address";
    switch (operation) {
    case MathOperation::Add:
    case MathOperation::Sub:
    case MathOperation::Min:
    case MathOperation::Max:
    case MathOperation::And:
    case MathOperation::Or:
    case MathOperation::Xor:
    case MathOperation::Xchg:
        macroBody << ",other";
        break;
    case MathOperation::Inc:
    case MathOperation::Dec:
        break;
    case MathOperation::CmpXchg:
        macroBody << ",other,other";
        break;
    default:
        FATAL_ERROR("Unknown atomic operation");
    }
    macroBody << ")";

    options.addMacro("ATOMIC_OP", {"address", "other"}, macroBody.str().c_str());
}

void KernelHelper::addExplicitAtomicOpMacro(CompilerOptionsBuilder &options, MathOperation operation, AtomicMemoryOrder order, AtomicScope scope) {
    const static char *functionNames[] = {"ERROR",
                                          "atomic_fetch_add_explicit", "atomic_fetch_sub_explicit",
                                          "atomic_exchange_explicit", "atomic_compare_exchange_strong_explicit",
                                          "atomic_fetch_add_explicit", "atomic_fetch_sub_explicit", // inc and dec emulated with add and sub
                                          "atomic_fetch_min_explicit", "atomic_fetch_max_explicit",
                                          "atomic_fetch_and_explicit", "atomic_fetch_or_explicit",
                                          "atomic_fetch_xor_explicit"};

    std::ostringstream macroBody{};
    macroBody << functionNames[static_cast<int>(operation)] << "(address";
    switch (operation) {
    case MathOperation::Add:
    case MathOperation::Sub:
    case MathOperation::Min:
    case MathOperation::Max:
    case MathOperation::And:
    case MathOperation::Or:
    case MathOperation::Xor:
    case MathOperation::Xchg:
        macroBody << ",other";
        break;
    case MathOperation::Inc:
    case MathOperation::Dec:
        macroBody << ",1";
        break;
    case MathOperation::CmpXchg:
        macroBody << ",&other,other," << AtomicMemoryOrderHelper::toOpenclC(order);
        break;
    default:
        FATAL_ERROR("Unknown atomic operation");
    }
    macroBody << "," << AtomicMemoryOrderHelper::toOpenclC(order)
              << "," << AtomicScopeHelper::toOpenclC(scope)
              << ")";

    options.addMacro("ATOMIC_OP", {"address", "other"}, macroBody.str().c_str());
}
