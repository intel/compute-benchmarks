/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ocl/opencl.h"
#include "framework/ocl/utility/usm_helper_ocl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_first_cpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedFirstCpuAccessArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Opencl opencl;
    Timer timer;
    auto clSharedMemAllocINTEL = (pfn_clSharedMemAllocINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clSharedMemAllocINTEL");
    auto clMemFreeINTEL = (pfn_clMemFreeINTEL)clGetExtensionFunctionAddressForPlatform(opencl.platform, "clMemFreeINTEL");
    if (!clSharedMemAllocINTEL || !clMemFreeINTEL) {
        return TestResult::DriverFunctionNotFound;
    }
    cl_int retVal{};

    // Warmup
    const cl_mem_properties_intel properties[] = {
        CL_MEM_ALLOC_FLAGS_INTEL,
        UsmHelperOcl::getInitialPlacementFlag(arguments.initialPlacement),
        0,
    };
    auto buffer = clSharedMemAllocINTEL(opencl.context, opencl.device, properties, arguments.bufferSize, 0u, &retVal);
    ASSERT_CL_SUCCESS(retVal);
    static_cast<uint8_t *>(buffer)[0] = 0;
    ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, buffer));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        buffer = clSharedMemAllocINTEL(opencl.context, opencl.device, properties, arguments.bufferSize, 0u, &retVal);
        ASSERT_CL_SUCCESS(retVal);

        timer.measureStart();
        static_cast<uint32_t *>(buffer)[0] = 0;
        timer.measureEnd();

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        ASSERT_CL_SUCCESS(clMemFreeINTEL(opencl.context, buffer));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedFirstCpuAccess> registerTestCase(run, Api::OpenCL, true);
