/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_unmap.h"

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

static TestResult doVirtualMemUnMap(LevelZero &levelzero, const VirtualMemUnMapArguments &arguments, Timer &timer) {

    size_t pageSize = 0;
    size_t reserveSize = arguments.reserveSize;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.reserveSize, &pageSize));
    reserveSize = getPageAlignedSize(reserveSize, pageSize);

    // Allocate Physical Memory handle
    ze_physical_mem_desc_t physDesc = {ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC, nullptr};
    physDesc.size = reserveSize;
    ze_physical_mem_handle_t physicalMemoryHandle;
    ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemCreate(levelzero.context, levelzero.device, &physDesc, &physicalMemoryHandle));

    // Reserve virtual Memory
    void *virtualMem = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, reserveSize, &virtualMem));
    EXPECT_NE(virtualMem, static_cast<void *>(nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemMap(levelzero.context, virtualMem, reserveSize,
                                             physicalMemoryHandle, 0,
                                             ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));

    timer.measureStart();
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemUnmap(levelzero.context, virtualMem, reserveSize));
    timer.measureEnd();

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, virtualMem, reserveSize));
    ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemDestroy(levelzero.context, physicalMemoryHandle));

    return TestResult::Success;
}

static TestResult run(const VirtualMemUnMapArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Warmup
    auto status = doVirtualMemUnMap(levelzero, arguments, timer);
    if (status != TestResult::Success) {
        return status;
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        status = doVirtualMemUnMap(levelzero, arguments, timer);
        if (status != TestResult::Success) {
            return status;
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemUnMap> registerTestCase(run, Api::L0);
