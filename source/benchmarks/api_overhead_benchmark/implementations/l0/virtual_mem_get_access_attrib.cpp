/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_get_access_attrib.h"

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

static TestResult doVirtualMemGetAccessAttrib(LevelZero &levelzero, const VirtualMemGetAccessAttribArguments &arguments, Timer &timer) {

    size_t pageSize = 0;
    size_t size = arguments.size;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.size, &pageSize));
    size = getPageAlignedSize(size, pageSize);

    // Reserve virtual Memory
    void *virtualMem = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, size * 2, &virtualMem));
    EXPECT_NE(virtualMem, static_cast<void *>(nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemSetAccessAttribute(levelzero.context, virtualMem, size,
                                                            ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));

    timer.measureStart();
    size_t outSize = 0;
    ze_memory_access_attribute_t getAccessType;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemGetAccessAttribute(levelzero.context,
                                                            virtualMem, size,
                                                            &getAccessType, &outSize));
    timer.measureEnd();

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, virtualMem, size * 2));

    return TestResult::Success;
}

static TestResult run(const VirtualMemGetAccessAttribArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        auto status = doVirtualMemGetAccessAttrib(levelzero, arguments, timer);
        if (status != TestResult::Success) {
            return status;
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemGetAccessAttrib> registerTestCase(run, Api::L0);
