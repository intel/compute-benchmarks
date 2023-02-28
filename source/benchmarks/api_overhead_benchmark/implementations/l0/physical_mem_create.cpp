/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/physical_mem_create.h"

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include <gtest/gtest.h>

static size_t getPageAlignedSize(size_t requestedSize, size_t pageSize) {
    if (pageSize >= requestedSize) {
        return pageSize;
    } else {
        return (requestedSize / pageSize) * pageSize;
    }
}

static TestResult doPhysicalMemCreate(LevelZero &levelzero, const PhysicalMemCreateArguments &arguments, Timer &timer) {

    size_t pageSize = 0;
    auto reserveSize = arguments.reserveSize;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.reserveSize, &pageSize));
    reserveSize = getPageAlignedSize(reserveSize, pageSize);
    ze_physical_mem_desc_t physDesc = {ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC,
                                       nullptr};
    physDesc.size = reserveSize;
    ze_physical_mem_handle_t physicalMemoryHandle;
    timer.measureStart();
    ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemCreate(levelzero.context, levelzero.device, &physDesc, &physicalMemoryHandle));
    timer.measureEnd();
    EXPECT_NE(physicalMemoryHandle, static_cast<void *>(nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemDestroy(levelzero.context, physicalMemoryHandle));

    return TestResult::Success;
}

static TestResult run(const PhysicalMemCreateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Warmup
    auto status = doPhysicalMemCreate(levelzero, arguments, timer);
    if (status != TestResult::Success) {
        return status;
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        status = doPhysicalMemCreate(levelzero, arguments, timer);
        if (status != TestResult::Success) {
            return status;
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<PhysicalMemCreate> registerTestCase(run, Api::L0);
