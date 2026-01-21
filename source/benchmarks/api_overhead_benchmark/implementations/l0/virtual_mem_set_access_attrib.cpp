/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_set_access_attrib.h"

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

static TestResult doVirtualMemSetAccessAttrib(LevelZero &levelzero, const VirtualMemSetAccessAttribArguments &arguments, Timer &timer) {

    size_t pageSize = 0;
    size_t size = arguments.size;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.size, &pageSize));
    size = getPageAlignedSize(size, pageSize);

    // Reserve virtual Memory
    void *virtualMem = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, size, &virtualMem));
    EXPECT_NE(virtualMem, static_cast<void *>(nullptr));

    // Handle access type
    ze_memory_access_attribute_t accessType = ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE;
    std::string accessTypeString = static_cast<const std::string &>(arguments.accessType);
    if (accessTypeString == "ReadOnly") {
        accessType = ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY;
    } else if (accessTypeString == "None") {
        accessType = ZE_MEMORY_ACCESS_ATTRIBUTE_NONE;
    }

    timer.measureStart();
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemSetAccessAttribute(levelzero.context, virtualMem, size,
                                                            accessType));
    timer.measureEnd();

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, virtualMem, size));

    return TestResult::Success;
}

static TestResult run(const VirtualMemSetAccessAttribArguments &arguments, Statistics &statistics) {
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
        auto status = doVirtualMemSetAccessAttrib(levelzero, arguments, timer);
        if (status != TestResult::Success) {
            return status;
        }
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemSetAccessAttrib> registerTestCase(run, Api::L0);
