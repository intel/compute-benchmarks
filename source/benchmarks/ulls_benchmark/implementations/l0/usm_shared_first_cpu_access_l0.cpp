/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_first_cpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedFirstCpuAccessArguments &arguments, Statistics &statistics) {
    LevelZero levelzero;
    Timer timer;

    // Prepare buffer descriptions
    ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    if (arguments.initialPlacement == UsmInitialPlacement::Host) {
        hostAllocationDesc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT;
    }
    ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    if (arguments.initialPlacement == UsmInitialPlacement::Device) {
        deviceAllocationDesc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT;
    }
    void *buffer{};

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(levelzero.context, &deviceAllocationDesc, &hostAllocationDesc, arguments.bufferSize, 0, levelzero.device, &buffer));
    static_cast<uint8_t *>(buffer)[0] = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(levelzero.context, &deviceAllocationDesc, &hostAllocationDesc, arguments.bufferSize, 0, levelzero.device, &buffer));

        timer.measureStart();
        static_cast<uint32_t *>(buffer)[0] = 0;
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedFirstCpuAccess> registerTestCase(run, Api::L0);
