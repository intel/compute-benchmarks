/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/test_case/register_test_case.h"
#include "framework/ur/error.h"
#include "framework/ur/ur.h"
#include "framework/ur/usm_helper.h"
#include "framework/utility/random_distribution.h"
#include "framework/utility/timer.h"

#include "definitions/usm_random_memory_allocation.h"

#include <gtest/gtest.h>
#include <random>

static TestResult run(const UsmRandomMemoryAllocationArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    UrState ur;
    Timer timer;

    auto size = makeRandomDistribution(arguments.sizeDistribution, arguments.minSize, arguments.maxSize);
    auto operation = std::uniform_int_distribution<int>{0, 1};

    // Warmup
    std::vector<void *> ptrs;
    std::mt19937 gen{0};
    ptrs.reserve(arguments.operationCount * arguments.iterations);
    ptrs.resize(arguments.operationCount);
    for (auto &ptr : ptrs)
        ASSERT_UR_RESULT_SUCCESS(UR::UsmHelper::allocate(arguments.usmMemoryPlacement, ur.context, ur.device, size->get(gen), &ptr));

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();

        if (operation(gen)) { // alloc
            void *ptr;
            ASSERT_UR_RESULT_SUCCESS(UR::UsmHelper::allocate(arguments.usmMemoryPlacement, ur.context, ur.device, size->get(gen), &ptr));
            ptrs.push_back(ptr);
        } else { // free
            assert(!ptrs.empty());
            size_t idx = std::uniform_int_distribution<size_t>{0, ptrs.size() - 1}(gen);
            std::swap(ptrs[idx], ptrs.back());
            ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, ptrs.back()));
            ptrs.pop_back();
        }

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (auto ptr : ptrs)
        ASSERT_UR_RESULT_SUCCESS(urUSMFree(ur.context, ptr));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmRandomMemoryAllocation> registerTestCase(run, Api::UR);
