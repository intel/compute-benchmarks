/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/get_memory_properties_with_offseted_pointer.h"

#include <gtest/gtest.h>

static TestResult run(const GetMemoryPropertiesWithOffsetedPointerArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    constexpr uint32_t offset = 2u;

    // Setup
    LevelZero levelzero;
    Timer timer;

    std::vector<void *> allocations;
    allocations.reserve(arguments.AllocationsCount);

    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
        void *ptr = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, sizeof(int32_t), 4u, levelzero.device, &ptr));
        allocations.push_back(ptr);
    }

    // Warmup
    ze_memory_allocation_properties_t properties{};
    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        auto ptr = static_cast<uint8_t *>(allocations[i]) + offset;
        ASSERT_ZE_RESULT_SUCCESS(zeMemGetAllocProperties(levelzero.context, ptr, &properties, nullptr));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        timer.measureStart();
        for (int64_t j = 0; j < arguments.AllocationsCount; ++j) {
            auto ptr = static_cast<uint8_t *>(allocations[j]) + offset;
            ASSERT_ZE_RESULT_SUCCESS(zeMemGetAllocProperties(levelzero.context, ptr, &properties, nullptr));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get() / arguments.AllocationsCount, typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, allocations[i]));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<GetMemoryPropertiesWithOffsetedPointer> registerTestCase(run, Api::L0);
