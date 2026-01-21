/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/set_kernel_arg_svm_pointer.h"

#include <gtest/gtest.h>

static TestResult run(const SetKernelArgSvmPointerArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Opencl opencl;
    Timer timer;
    cl_int retVal{};
    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DriverFunctionNotFound;
    }

    // Create kernels
    const std::vector<uint8_t> kernelSource = FileHelper::loadTextFile("api_overhead_benchmark_fill_with_ones.cl");
    if (kernelSource.size() == 0) {
        return TestResult::KernelNotFound;
    }
    const char *source = reinterpret_cast<const char *>(kernelSource.data());
    const size_t sourceLength = kernelSource.size();
    cl_program program = clCreateProgramWithSource(opencl.context, 1, &source, &sourceLength, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    ASSERT_CL_SUCCESS(clBuildProgram(program, 1, &opencl.device, nullptr, nullptr, nullptr));

    std::vector<cl_kernel> kernels;
    for (auto i = 0u; i < arguments.allocationsCount; i++) {
        cl_kernel kernel = clCreateKernel(program, "fill_with_ones", &retVal);
        ASSERT_CL_SUCCESS(retVal);
        kernels.push_back(kernel);
    }

    // Create allocations
    std::vector<UsmHelperOcl::Alloc> allocations;
    for (auto i = 0u; i < arguments.allocationsCount; i++) {
        UsmHelperOcl::Alloc alloc{};
        ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Shared, arguments.allocationSize, alloc));
        allocations.push_back(alloc);
    }

    // Reallocate if argument is set
    if (arguments.reallocate) {
        for (auto i = 0u; i < arguments.allocationsCount; i++) {
            ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(allocations[i]));
            UsmHelperOcl::Alloc alloc{};
            ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Shared, arguments.allocationSize, alloc));
            allocations[i] = alloc;
        }
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto j = 0u; j < arguments.allocationsCount; j++) {
            ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernels[j], 0, static_cast<cl_int *>(allocations[j].ptr)));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (auto i = 0u; i < arguments.allocationsCount; i++) {
        ASSERT_CL_SUCCESS(clReleaseKernel(kernels[i]));
        ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(allocations[i]));
    }
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<SetKernelArgSvmPointer> registerTestCase(run, Api::OpenCL);
