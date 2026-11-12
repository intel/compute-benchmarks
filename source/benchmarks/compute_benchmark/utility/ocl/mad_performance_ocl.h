/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/test_case_argument_container.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include "../mad_performance.h"

#include <string>
#include <vector>

inline bool isMadDataTypeSupported(OCL::Opencl &opencl, MadPerformance::DataType dataType) {
    if (dataType == MadPerformance::DataType::Half) {
        return opencl.getExtensions().isSupported("cl_khr_fp16", false);
    }
    if (dataType == MadPerformance::DataType::Double) {
        return opencl.getExtensions().areDoublesSupported();
    }
    return true;
}

inline TestResult runMadPerformanceOcl(const TestCaseArgumentContainer &arguments, Statistics &statistics, MadPerformance::DataType dataType,
                                       size_t vectorWidth, bool useEvents) {
    const MeasurementFields typeSelector(MadPerformance::getMeasurementUnit(dataType), useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    const OCL::QueueProperties queueProperties = OCL::QueueProperties::create().setProfiling(useEvents);
    OCL::Opencl opencl(queueProperties);

    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DeviceNotCapable;
    }
    if (!isMadDataTypeSupported(opencl, dataType)) {
        return TestResult::DeviceNotCapable;
    }

    cl_event profilingEvent{};
    cl_event *pProfilingEvent = useEvents ? &profilingEvent : nullptr;
    Timer timer{};
    cl_int retVal{};

    size_t workgroupSize = 0;
    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(workgroupSize), &workgroupSize, nullptr));
    cl_uint euCount = 0;
    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(euCount), &euCount, nullptr));
    const size_t workgroupCount = euCount * MadPerformance::threadsPerEu * MadPerformance::wavesCount;
    const size_t gws = workgroupSize * workgroupCount;
    const size_t bufferSize = gws * vectorWidth * MadPerformance::getScalarSize(dataType);
    const uint64_t totalOperations = MadPerformance::getOperationsPerKernel(gws);

    const std::vector<uint8_t> multiplier = MadPerformance::getOne(dataType);
    const std::vector<uint8_t> addend = MadPerformance::getOne(dataType);
    const uint32_t loopIterations = MadPerformance::getLoopIterations(vectorWidth);
    const uint8_t initialValue = 0u;

    auto clEnqueueMemFillINTEL = reinterpret_cast<pfn_clEnqueueMemFillINTEL>(clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemFillINTEL"));
    auto clEnqueueMemcpyINTEL = reinterpret_cast<pfn_clEnqueueMemcpyINTEL>(clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemcpyINTEL"));
    if (!clEnqueueMemFillINTEL || !clEnqueueMemcpyINTEL) {
        return TestResult::DriverFunctionNotFound;
    }

    UsmHelperOcl::Alloc bufferAlloc{};
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Device, bufferSize, bufferAlloc));

    cl_program program = nullptr;
    const std::string buildOptions = MadPerformance::getBuildOptions(dataType, vectorWidth);
    if (auto result = OCL::ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, "compute_benchmark_mad_performance.cl", buildOptions.c_str(), program); result != TestResult::Success) {
        ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(bufferAlloc));
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, "mad_performance", &retVal);
    ASSERT_CL_SUCCESS(retVal);

    ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 0, bufferAlloc.ptr));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, multiplier.size(), multiplier.data()));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, addend.size(), addend.data()));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 3, sizeof(loopIterations), &loopIterations));

    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_CL_SUCCESS(clEnqueueMemFillINTEL(opencl.commandQueue, bufferAlloc.ptr, &initialValue, sizeof(initialValue), bufferSize, 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &workgroupSize, 0, nullptr, pProfilingEvent));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (pProfilingEvent) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), totalOperations, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), totalOperations, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    std::vector<uint8_t> result(bufferSize);
    ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(opencl.commandQueue, CL_TRUE, result.data(), bufferAlloc.ptr, bufferSize, 0, nullptr, nullptr));
    const auto testResult = MadPerformance::verify(dataType, vectorWidth, result) ? TestResult::Success : TestResult::VerificationFail;

    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(bufferAlloc));

    return testResult;
}
