/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/black_scholes.h"
#include "utility/workload_input.h"

#include <type_traits>

using namespace OCL;

template <typename ValueType>
static TestResult runWorkload(const BlackScholesArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Opencl opencl{};
    if constexpr (std::is_same_v<ValueType, double>) {
        if (!opencl.getExtensions().areDoublesSupported()) {
            return TestResult::DeviceNotCapable;
        }
    }

    cl_int retVal{};

    const size_t bufferSize = BlackScholes::optionCount * sizeof(ValueType);
    const auto options = WorkloadInput::generateBlackScholesOptions<ValueType>(BlackScholes::optionCount);
    std::vector<ValueType> call(BlackScholes::optionCount);
    std::vector<ValueType> put(BlackScholes::optionCount);

    const char *buildOptions = std::is_same_v<ValueType, double> ? "-DDATA_TYPE_DOUBLE" : nullptr;
    cl_program program{};
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "algorithm_benchmark_black_scholes.cl", buildOptions, program); result != TestResult::Success) {
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "black_scholes", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    cl_mem yearsBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_ONLY, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem strikeBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_ONLY, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem stockPriceBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_ONLY, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem callBuffer = clCreateBuffer(opencl.context, CL_MEM_READ_WRITE, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    cl_mem putBuffer = clCreateBuffer(opencl.context, CL_MEM_WRITE_ONLY, bufferSize, nullptr, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const ValueType riskFree = static_cast<ValueType>(BlackScholes::riskFree);
    const ValueType volatility = static_cast<ValueType>(BlackScholes::volatility);
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 0, sizeof(riskFree), &riskFree));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(volatility), &volatility));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(yearsBuffer), &yearsBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 3, sizeof(strikeBuffer), &strikeBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 4, sizeof(stockPriceBuffer), &stockPriceBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 5, sizeof(callBuffer), &callBuffer));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 6, sizeof(putBuffer), &putBuffer));

    const size_t globalSize = BlackScholes::optionCount;
    const size_t localSize = BlackScholes::groupSize;
    Timer timer{};
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, yearsBuffer, CL_FALSE, 0, bufferSize, options.years.data(), 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, strikeBuffer, CL_FALSE, 0, bufferSize, options.strike.data(), 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueWriteBuffer(opencl.commandQueue, stockPriceBuffer, CL_FALSE, 0, bufferSize, options.stockPrice.data(), 0, nullptr, nullptr));
        for (auto launch = 0u; launch < BlackScholes::kernelLaunchCount; launch++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &globalSize, &localSize, 0, nullptr, nullptr));
        }
        ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, callBuffer, CL_FALSE, 0, bufferSize, call.data(), 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clEnqueueReadBuffer(opencl.commandQueue, putBuffer, CL_TRUE, 0, bufferSize, put.data(), 0, nullptr, nullptr));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (cl_mem buffer : {yearsBuffer, strikeBuffer, stockPriceBuffer, callBuffer, putBuffer}) {
        ASSERT_CL_SUCCESS(clReleaseMemObject(buffer));
    }
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static TestResult run(const BlackScholesArguments &arguments, Statistics &statistics) {
    if (arguments.dataType == DataType::Float) {
        return runWorkload<float>(arguments, statistics);
    }
    if (arguments.dataType == DataType::Double) {
        return runWorkload<double>(arguments, statistics);
    }
    return TestResult::InvalidArgs;
}

static RegisterTestCaseImplementation<BlackScholes> registerTestCase(run, Api::OpenCL);
