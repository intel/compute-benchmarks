/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/profiling_helper.h"
#include "framework/ocl/utility/program_helper_ocl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/single_precision_performance.h"

#include <gtest/gtest.h>

static TestResult run(const SinglePrecisionPerformanceArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigaFLOPS, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().setProfiling(arguments.useEvents);
    Opencl opencl(queueProperties);

    if (!opencl.getExtensions().isUsmSupported()) {
        return TestResult::DeviceNotCapable;
    }

    cl_event profilingEvent{};
    cl_event *pProfilingEvent = arguments.useEvents ? &profilingEvent : nullptr;
    Timer timer{};
    cl_int retVal{};

    size_t maxWorkgroupSize = 0;
    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(maxWorkgroupSize), &maxWorkgroupSize, nullptr));
    const size_t workgroupSize = maxWorkgroupSize;

    size_t euCount = 0;
    ASSERT_CL_SUCCESS(clGetDeviceInfo(opencl.device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(euCount), &euCount, nullptr));
    const size_t threadsPerEu = 8u;
    const size_t wavesCount = 4u;
    const size_t workgroupCount = euCount * threadsPerEu * wavesCount;
    const size_t gws = workgroupSize * workgroupCount;

    const size_t loopIterations = 200u;
    const size_t opsPerLoop = 128u;
    const size_t flopsPerOp = 2u;
    const uint64_t totalFlops = static_cast<uint64_t>(gws) * loopIterations * opsPerLoop * flopsPerOp;

    const float multiplier = 1.0f;
    const float addend = 1.0f;
    const float initialValue = 0.0f;
    const float expectedValue = static_cast<float>(loopIterations * opsPerLoop);

    auto clEnqueueMemFillINTEL = (pfn_clEnqueueMemFillINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemFillINTEL");
    auto clEnqueueMemcpyINTEL = (pfn_clEnqueueMemcpyINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clEnqueueMemcpyINTEL");
    if (!clEnqueueMemFillINTEL || !clEnqueueMemcpyINTEL) {
        return TestResult::DriverFunctionNotFound;
    }

    UsmHelperOcl::Alloc bufferAlloc{};
    ASSERT_CL_SUCCESS(UsmHelperOcl::allocate(opencl, UsmMemoryPlacement::Device, gws * sizeof(float), bufferAlloc));

    cl_program program = nullptr;
    const char *programName = "compute_benchmark_single_precision_performance.cl";
    const char *kernelName = "single_precision_performance";
    if (auto result = ProgramHelperOcl::buildProgramFromSourceFile(opencl.context, opencl.device, programName, "", program); result != TestResult::Success) {
        ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(bufferAlloc));
        return result;
    }
    cl_kernel kernel = clCreateKernel(program, kernelName, &retVal);
    ASSERT_CL_SUCCESS(retVal);

    const uint32_t loopIterationsUint = static_cast<uint32_t>(loopIterations);
    ASSERT_CL_SUCCESS(clSetKernelArgSVMPointer(kernel, 0, bufferAlloc.ptr));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 1, sizeof(multiplier), &multiplier));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 2, sizeof(addend), &addend));
    ASSERT_CL_SUCCESS(clSetKernelArg(kernel, 3, sizeof(loopIterationsUint), &loopIterationsUint));

    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_CL_SUCCESS(clEnqueueMemFillINTEL(opencl.commandQueue, bufferAlloc.ptr, &initialValue, sizeof(float), gws * sizeof(float), 0, nullptr, nullptr));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));

        timer.measureStart();
        ASSERT_CL_SUCCESS(clEnqueueNDRangeKernel(opencl.commandQueue, kernel, 1, nullptr, &gws, &workgroupSize, 0, nullptr, pProfilingEvent));
        ASSERT_CL_SUCCESS(clFinish(opencl.commandQueue));
        timer.measureEnd();

        if (pProfilingEvent) {
            cl_ulong timeNs{};
            ASSERT_CL_SUCCESS(ProfilingHelper::getEventDurationInNanoseconds(profilingEvent, timeNs));
            ASSERT_CL_SUCCESS(clReleaseEvent(profilingEvent));
            statistics.pushValue(std::chrono::nanoseconds(timeNs), totalFlops, typeSelector.getUnit(), typeSelector.getType());
        } else {
            statistics.pushValue(timer.get(), totalFlops, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    auto testResult = TestResult::Success;
    std::vector<float> result(gws);
    ASSERT_CL_SUCCESS(clEnqueueMemcpyINTEL(opencl.commandQueue, CL_TRUE, result.data(), bufferAlloc.ptr, gws * sizeof(float), 0, nullptr, nullptr));
    for (size_t i = 0; i < gws; i++) {
        if (result[i] != expectedValue) {
            testResult = TestResult::VerificationFail;
            break;
        }
    }

    ASSERT_CL_SUCCESS(clReleaseKernel(kernel));
    ASSERT_CL_SUCCESS(clReleaseProgram(program));
    ASSERT_CL_SUCCESS(UsmHelperOcl::deallocate(bufferAlloc));

    return testResult;
}

static RegisterTestCaseImplementation<SinglePrecisionPerformance> registerTestCase(run, Api::OpenCL);
