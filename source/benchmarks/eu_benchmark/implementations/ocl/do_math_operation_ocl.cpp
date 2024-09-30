/*
 * Copyright (C) 2022-2024 Intel Corporation
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

#include <cstring>
#include <gtest/gtest.h>

std::string getCompilerOptions(DataType dataType, MathOperation operation, bool largeGrfMode) {
    CompilerOptionsBuilder options{};
    options.addDefinitionKeyValue("DATATYPE", DataTypeHelper::toOpenclC(dataType));

    switch (operation) {
    case MathOperation::Add:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a+b");
        break;
    case MathOperation::Sub:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a-b");
        break;
    case MathOperation::Div:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a/b");
        break;
    case MathOperation::Modulo:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a%b");
        break;
    case MathOperation::Inc:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a++");
        break;
    case MathOperation::Dec:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a--");
        break;
    case MathOperation::Min:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=min(a,b)");
        break;
    case MathOperation::Max:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=max(a,b)");
        break;
    case MathOperation::And:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a&b");
        break;
    case MathOperation::Or:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a|b");
        break;
    case MathOperation::Xor:
        options.addMacro("MATH_OPERATION", {"a", "b"}, "a=a^b");
        break;
    default:
        FATAL_ERROR("Invalid math operation");
    }

    if (largeGrfMode) {
        options.addOption("-cl-intel-256-GRF-per-thread");
    }

    return options.str();
}

static TestResult run(const DoMathOperationArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Check support
    if (!MathOperationHelper::isSupportedAsNormal(arguments.operation, arguments.dataType)) {
        return TestResult::DeviceNotCapable;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);
    cl_event profilingEvent{};
    cl_event *eventForEnqueue = arguments.useEvents ? &profilingEvent : nullptr;
    Timer timer{};
    cl_int retVal{};

    // Prepare data
    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const size_t loopIterations = 200u;                   // tweakable constant
    const size_t operationsPerLoop = 128u;                // non-tweakable constant, this is hardcoded in the kernel code
    const size_t enqueueCount = arguments.iterations + 1; // non-tweakable constant
    const auto data = MathOperationHelper::generateTestData(arguments.dataType, arguments.operation, loopIterations, operationsPerLoop, enqueueCount);

    // Create and initialize the buffer with test data
    cl_mem buffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, gws * data.sizeOfDataType, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, buffer, CL_FALSE, 0, data.sizeOfDataType, data.initialValue, 0, nullptr, nullptr));

    // Create kernel
    cl_program program = nullptr;
    const char *programName = "eu_benchmark_perform_math_operation.cl";
    const char *kernelName = "do_math_operation";
    const std::string compilerOptions = getCompilerOptions(arguments.dataType, arguments.operation, false);
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, compilerOptions.c_str(), program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_program largeGrfProgram = nullptr;
    cl_kernel largeGrfKernel = nullptr;

    if (arguments.mixGrfModes) {
        const std::string largeGrfCompilerOptions = getCompilerOptions(arguments.dataType, arguments.operation, true);
        if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, largeGrfCompilerOptions.c_str(), largeGrfProgram); result != TestResult::Success) {
            return result;
        }
        largeGrfKernel = clCreateKernel(largeGrfProgram, kernelName, &retVal);
        ASSERT_CL_SUCCESS(retVal);

        ASSERT_CL_SUCCESS(clSetKernelArg(largeGrfKernel, 0, sizeof(buffer), &buffer));
        ASSERT_CL_SUCCESS(clSetKernelArg(largeGrfKernel, 1, data.sizeOfDataType, data.otherArgument));
        ASSERT_CL_SUCCESS(clSetKernelArg(largeGrfKernel, 2, sizeof(data.loopIterations), &data.loopIterations));
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, largeGrfKernel, 1, nullptr, &gws, &lws, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        if (eventForEnqueue) {
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
        }
    }

    // Warmup
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, data.sizeOfDataType, data.otherArgument));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(data.loopIterations), &data.loopIterations));
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &lws, 0, nullptr, eventForEnqueue));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
    if (eventForEnqueue) {
        ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        auto kernelForExecution = kernel;
        if (arguments.mixGrfModes && i % 2 == 0) {
            kernelForExecution = largeGrfKernel;
        } else {
            kernelForExecution = kernel;
        }

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernelForExecution, 1, nullptr, &gws, &lws, 0, nullptr, eventForEnqueue));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();
        if (eventForEnqueue) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Verify
    std::byte result[8] = {};
    ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, buffer, CL_BLOCKING, 0, data.sizeOfDataType, result, 0, nullptr, nullptr));
    if (std::memcmp(result, data.expectedValue, data.sizeOfDataType) != 0) {
        return TestResult::VerificationFail;
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));

    if (arguments.mixGrfModes) {
        ASSERT_CL_SUCCESS(clReleaseKernel(largeGrfKernel));
        ASSERT_CL_SUCCESS(clReleaseProgram(largeGrfProgram));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<DoMathOperation> registerTestCase(run, Api::OpenCL);
