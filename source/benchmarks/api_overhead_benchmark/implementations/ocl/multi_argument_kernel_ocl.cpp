/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/multi_argument_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const MultiArgumentKernelTimeArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (arguments.useL0NewArgApi) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Opencl opencl;
    cl_int retVal{};
    Timer timer;

    size_t gws[3] = {arguments.lws * arguments.groupCount, 1, 1};
    const size_t lws[3] = {arguments.lws, 1, 1};

    // Create kernels
    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile("api_overhead_benchmark_multi_arg_kernel.cl");
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));

    std::string kernelName = "kernelWith" + std::to_string(arguments.argumentCount);

    if (arguments.useGlobalIds) {
        kernelName += "WithIds";
    }

    cl_kernel kernel = clCreateKernel(program, kernelName.c_str(), &retVal);
    ASSERT_CL_SUCCESS(retVal);

    std::vector<UsmHelperOcl::Alloc> allocations;
    allocations.reserve(arguments.argumentCount);
    for (auto argumentId = 0u; argumentId < arguments.argumentCount; argumentId++) {
        UsmHelperOcl::Alloc alloc{};
        ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Device, 4096u, alloc));
        allocations.push_back(alloc);
        ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, argumentId, alloc.ptr));
    }

    // Warmup, kernel
    ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 3, nullptr, gws, lws, 0, nullptr, nullptr));
    ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

    auto reverseOrder = false;

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.measureSetKernelArg) {
            timer.measureStart();
        }
        if (arguments.reverseOrder) {
            reverseOrder = !reverseOrder;
        }

        for (auto argumentId = 0u; argumentId < arguments.argumentCount; argumentId++) {
            ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, argumentId, allocations[reverseOrder ? arguments.argumentCount - argumentId - 1 : argumentId].ptr));
        }

        if (!arguments.measureSetKernelArg) {
            timer.measureStart();
        }
        for (auto index = 0u; index < arguments.count; index++) {
            ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 3, nullptr, gws, lws, 0, nullptr, nullptr));
        }
        timer.measureEnd();
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));

    for (auto &alloc : allocations) {
        ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(alloc));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiArgumentKernelTime> registerTestCase(run, Api::OpenCL);
