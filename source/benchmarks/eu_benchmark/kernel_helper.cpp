/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "kernel_helper.h"

#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/error.h"
#include "framework/utility/math_operation_helper.h"

#include "definitions/do_math_operation.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <type_traits>

constexpr size_t operationsPerPair = 2;

template <typename DataTypeT>
static DataTypeT readValue(const std::byte *source) {
    DataTypeT value{};
    std::memcpy(&value, source, sizeof(DataTypeT));
    return value;
}

OperationChain KernelHelper::getOperationChain(const DataType dataType, const MathOperation operation) {
    switch (operation) {
    case MathOperation::Add:
    case MathOperation::Sub:
        return DataTypeHelper::isIntegerType(dataType) ? OperationChain::Coupled : OperationChain::Repeated;
    case MathOperation::Modulo:
    case MathOperation::Min:
    case MathOperation::Max:
    case MathOperation::And:
    case MathOperation::Or:
    case MathOperation::Xor:
        return OperationChain::Mixed;
    default:
        return OperationChain::Repeated;
    }
}

MathOperation KernelHelper::getMixingOperation(const MathOperation operation) {
    return operation == MathOperation::Max ? MathOperation::Sub : MathOperation::Add;
}

std::string KernelHelper::getAdditiveExpression(const DataType dataType, const MathOperation operation, const bool wrapping) {
    const char operatorSign = operation == MathOperation::Sub ? '-' : '+';
    if (!wrapping) {
        return std::string("a=a") + operatorSign + "b";
    }
    const std::string type = DataTypeHelper::toOpenclC(dataType);
    return "a=as_" + type + "(as_u" + type + "(a)" + operatorSign + "as_u" + type + "(b))";
}

std::string KernelHelper::getOperationExpression(const DataType dataType, const MathOperation operation, const bool wrapping) {
    switch (operation) {
    case MathOperation::Add:
    case MathOperation::Sub:
        return getAdditiveExpression(dataType, operation, wrapping);
    case MathOperation::Div:
        return "a=a/b";
    case MathOperation::Modulo:
        return "a=a%b";
    case MathOperation::Inc:
        return "a++";
    case MathOperation::Dec:
        return "a--";
    case MathOperation::Min:
        return "a=min(a,b)";
    case MathOperation::Max:
        return "a=max(a,b)";
    case MathOperation::And:
        return "a=a&b";
    case MathOperation::Or:
        return "a=a|b";
    case MathOperation::Xor:
        return "a=a^b";
    default:
        FATAL_ERROR("Invalid math operation");
    }
}

std::string KernelHelper::getCompilerOptions(const DataType dataType, const MathOperation operation, const OperationChain chain) {
    CompilerOptionsBuilder options{};
    options.addDefinitionKeyValue("DATATYPE", DataTypeHelper::toOpenclC(dataType));

    const bool wrapping = chain != OperationChain::Repeated && DataTypeHelper::isIntegerType(dataType);
    const std::string operationExpression = getOperationExpression(dataType, operation, wrapping);
    options.addMacro("MATH_OPERATION", {"a", "b"}, operationExpression.c_str());

    switch (chain) {
    case OperationChain::Repeated:
        options.addMacro("MATH_OPERATION_PAIR", {"a", "b"}, "MATH_OPERATION(a,b);MATH_OPERATION(a,b)");
        break;
    case OperationChain::Coupled:
        options.addMacro("MATH_OPERATION_PAIR", {"a", "b"}, "MATH_OPERATION(a,b);MATH_OPERATION(b,a)");
        break;
    case OperationChain::Mixed: {
        const std::string mixingExpression = getAdditiveExpression(dataType, getMixingOperation(operation), wrapping);
        options.addMacro("MIXING_OPERATION", {"a", "b"}, mixingExpression.c_str());
        options.addMacro("MATH_OPERATION_PAIR", {"a", "b"}, "MATH_OPERATION(a,b);MIXING_OPERATION(b,a);MATH_OPERATION(a,b);MIXING_OPERATION(b,a)");
        break;
    }
    }

    return options.str();
}

template <typename DataTypeT>
DataTypeT KernelHelper::applyOperation(const MathOperation operation, const DataTypeT value, const DataTypeT operand) {
    if constexpr (std::is_integral_v<DataTypeT>) {
        using UnsignedT = std::make_unsigned_t<DataTypeT>;
        switch (operation) {
        case MathOperation::Add:
            return static_cast<DataTypeT>(static_cast<UnsignedT>(value) + static_cast<UnsignedT>(operand));
        case MathOperation::Sub:
            return static_cast<DataTypeT>(static_cast<UnsignedT>(value) - static_cast<UnsignedT>(operand));
        case MathOperation::Modulo:
            return value % operand;
        case MathOperation::And:
            return value & operand;
        case MathOperation::Or:
            return value | operand;
        case MathOperation::Xor:
            return value ^ operand;
        default:
            break;
        }
    } else {
        switch (operation) {
        case MathOperation::Add:
            return value + operand;
        case MathOperation::Sub:
            return value - operand;
        default:
            break;
        }
    }

    switch (operation) {
    case MathOperation::Min:
        return std::min(value, operand);
    case MathOperation::Max:
        return std::max(value, operand);
    default:
        FATAL_ERROR("Invalid math operation");
    }
}

template <typename DataTypeT>
void KernelHelper::computeChainedResult(const MathOperation operation, const OperationChain chain, const size_t enqueueCount, const size_t pairsPerEnqueue, MathOperationTestData &data) {
    DataTypeT value = readValue<DataTypeT>(data.initialValue);
    const DataTypeT otherArgument = readValue<DataTypeT>(data.otherArgument);

    const MathOperation mixingOperation = getMixingOperation(operation);
    for (size_t enqueue = 0; enqueue < enqueueCount; enqueue++) {
        DataTypeT other = otherArgument;
        for (size_t pair = 0; pair < pairsPerEnqueue; pair++) {
            if (chain == OperationChain::Coupled) {
                value = applyOperation(operation, value, other);
                other = applyOperation(operation, other, value);
            } else {
                value = applyOperation(operation, value, other);
                other = applyOperation(mixingOperation, other, value);
                value = applyOperation(operation, value, other);
                other = applyOperation(mixingOperation, other, value);
            }
        }
    }

    std::memcpy(data.expectedValue, &value, sizeof(DataTypeT));
}

MathOperationTestData KernelHelper::getExpectedResult(const DoMathOperationArguments &arguments, const OperationChain chain, const size_t loopIterations,
                                                      const size_t operationsPerLoop, const size_t enqueueCount) {
    MathOperationTestData data = MathOperationHelper::generateTestData(arguments.dataType, arguments.operation, loopIterations, operationsPerLoop, enqueueCount);
    if (chain == OperationChain::Repeated) {
        return data;
    }

    const size_t pairsPerEnqueue = loopIterations * operationsPerLoop / operationsPerPair;
    switch (arguments.dataType) {
    case DataType::Int32:
        computeChainedResult<int32_t>(arguments.operation, chain, enqueueCount, pairsPerEnqueue, data);
        break;
    case DataType::Int64:
        computeChainedResult<int64_t>(arguments.operation, chain, enqueueCount, pairsPerEnqueue, data);
        break;
    case DataType::Float:
        computeChainedResult<float>(arguments.operation, chain, enqueueCount, pairsPerEnqueue, data);
        break;
    default:
        FATAL_ERROR("Invalid data type");
    }
    return data;
}
