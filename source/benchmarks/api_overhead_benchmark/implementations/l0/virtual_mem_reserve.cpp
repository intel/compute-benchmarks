/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_reserve.h"

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

static TestResult doVirtualMemReserve(LevelZero &levelzero, const VirtualMemReserveArguments &arguments, Timer &timer) {

    size_t pageSize = 0;
    auto reserveSize = arguments.reserveSize;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.reserveSize, &pageSize));
    reserveSize = getPageAlignedSize(reserveSize, pageSize);

    void *reservedPtr = nullptr;
    if (arguments.useNull) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, reserveSize, &reservedPtr));
        timer.measureEnd();
        EXPECT_NE(reservedPtr, static_cast<void *>(nullptr));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, reserveSize, &reservedPtr));
        EXPECT_NE(reservedPtr, static_cast<void *>(nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, reservedPtr, reserveSize));
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, reservedPtr, reserveSize, &reservedPtr));
        timer.measureEnd();
        EXPECT_NE(reservedPtr, static_cast<void *>(nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, reservedPtr, reserveSize));

    return TestResult::Success;
}

static TestResult run(const VirtualMemReserveArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Warmup
    auto status = doVirtualMemReserve(levelzero, arguments, timer);
    if (status != TestResult::Success) {
        return status;
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        status = doVirtualMemReserve(levelzero, arguments, timer);
        if (status != TestResult::Success) {
            return status;
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemReserve> registerTestCase(run, Api::L0);
