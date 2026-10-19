/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/math_operation_helper.h"
#include "framework/utility/timer.h"

#include "definitions/do_math_operation.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <gtest/gtest.h>
#include <type_traits>

using namespace OCL;

constexpr size_t operationsPerPair = 2;

enum class OperationChain {
    Repeated,
    Coupled,
    Mixed,
};

static OperationChain getOperationChain(const DataType dataType, const MathOperation operation) {
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

static MathOperation getMixingOperation(const MathOperation operation) {
    return operation == MathOperation::Max ? MathOperation::Sub : MathOperation::Add;
}

static std::string getAdditiveExpression(const DataType dataType, const MathOperation operation, const bool wrapping) {
    const char operatorSign = operation == MathOperation::Sub ? '-' : '+';
    if (!wrapping) {
        return std::string("a=a") + operatorSign + "b";
    }
    const std::string type = DataTypeHelper::toOpenclC(dataType);
    return "a=as_" + type + "(as_u" + type + "(a)" + operatorSign + "as_u" + type + "(b))";
}

static std::string getOperationExpression(const DataType dataType, const MathOperation operation, const bool wrapping) {
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

static std::string getCompilerOptions(const DataType dataType, const MathOperation operation, const OperationChain chain, const bool largeGrfMode) {
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

    if (largeGrfMode) {
        options.addOption("-cl-intel-256-GRF-per-thread");
    }

    return options.str();
}

template <typename DataTypeT>
static DataTypeT applyOperation(const MathOperation operation, const DataTypeT value, const DataTypeT operand) {
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
static DataTypeT readValue(const std::byte *source) {
    DataTypeT value{};
    std::memcpy(&value, source, sizeof(DataTypeT));
    return value;
}

template <typename DataTypeT>
static void computeChainedResult(const MathOperation operation, const OperationChain chain, const size_t enqueueCount, const size_t pairsPerEnqueue, MathOperationTestData &data) {
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

static MathOperationTestData getExpectedResult(const DoMathOperationArguments &arguments, const OperationChain chain, const size_t loopIterations,
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

static TestResult run(const DoMathOperationArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (!MathOperationHelper::isSupportedAsNormal(arguments.operation, arguments.dataType)) {
        return TestResult::DeviceNotCapable;
    }
    if (DataTypeHelper::isIntegerType(arguments.dataType) && MathOperationHelper::getArgumentsCount(arguments.operation) == 1) {
        return TestResult::NoImplementation;
    }
    const OperationChain chain = getOperationChain(arguments.dataType, arguments.operation);

    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);
    cl_event profilingEvent{};
    cl_event *pProfilingEvent = arguments.useEvents ? &profilingEvent : nullptr;
    Timer timer{};
    cl_int retVal{};

    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const size_t loopIterations = 200u;
    const size_t operationsPerLoop = 128u; // non-tweakable constant, this is hardcoded in the kernel code
    const size_t initIterations = arguments.mixGrfModes;
    const auto data = MathOperationHelper::generateTestData(arguments.dataType, arguments.operation, loopIterations, operationsPerLoop, 0);

    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, gws * data.sizeOfDataType, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    std::vector<std::byte> initialData(gws * data.sizeOfDataType, std::byte{0});
    for (size_t i = 0; i < gws; i++) {
        std::memcpy(initialData.data() + i * data.sizeOfDataType, data.initialValue, data.sizeOfDataType);
    }
    ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, initialData.size(), initialData.data(), 0, nullptr, nullptr));

    cl_program program = nullptr;
    const char *programName = "eu_benchmark_perform_math_operation.cl";
    const char *kernelName = "do_math_operation";
    const std::string compilerOptions = getCompilerOptions(arguments.dataType, arguments.operation, chain, false);
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.c_str(), program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_program largeGrfProgram = nullptr;
    cl_kernel largeGrfKernel = nullptr;

    if (arguments.mixGrfModes) {
        const std::string largeGrfCompilerOptions = getCompilerOptions(arguments.dataType, arguments.operation, chain, true);
        if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, largeGrfCompilerOptions.c_str(), largeGrfProgram); result != TestResult::Success) {
            return result;
        }
        largeGrfKernel = clCreateKernel(largeGrfProgram, kernelName, &retVal);
        ASSERT_CL_SUCCESS(retVal);

        ASSERT_CL_SUCCESS(clSetKernelArg(largeGrfKernel, 0, sizeof(buffer), &buffer));
        ASSERT_CL_SUCCESS(clSetKernelArg(largeGrfKernel, 1, data.sizeOfDataType, data.otherArgument));
        ASSERT_CL_SUCCESS(clSetKernelArg(largeGrfKernel, 2, sizeof(data.loopIterations), &data.loopIterations));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, largeGrfKernel, 1, nullptr, &gws, &lws, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    }

    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, data.sizeOfDataType, data.otherArgument));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(data.loopIterations), &data.loopIterations));
    for (auto i = 0u; i < arguments.iterations; i++) {
        auto kernelForExecution = kernel;
        if (arguments.mixGrfModes && i % 2 == 0) {
            kernelForExecution = largeGrfKernel;
        } else {
            kernelForExecution = kernel;
        }

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernelForExecution, 1, nullptr, &gws, &lws, 0, nullptr, pProfilingEvent));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        if (pProfilingEvent) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    const auto expected = getExpectedResult(arguments, chain, loopIterations, operationsPerLoop, arguments.iterations + initIterations);
    auto testResult = TestResult::Success;
    std::vector<std::byte> result(gws * data.sizeOfDataType);
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, result.size(), result.data(), 0, nullptr, nullptr));
    for (size_t i = 0; i < gws; i++) {
        if (std::memcmp(result.data() + i * data.sizeOfDataType, expected.expectedValue, data.sizeOfDataType) != 0) {
            testResult = TestResult::VerificationFail;
            break;
        }
    }

    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));

    if (arguments.mixGrfModes) {
        ASSERT_CL_SUCCESS(clReleaseKernel(largeGrfKernel));
        ASSERT_CL_SUCCESS(clReleaseProgram(largeGrfProgram));
    }

    return testResult;
}

static RegisterTestCaseImplementation<DoMathOperation> registerTestCase(run, Api::OpenCL);
