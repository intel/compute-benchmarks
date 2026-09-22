/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/ol/error.h"
#include "framework/ol/ol.h"
#include "framework/ol/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/random_distribution.h"
#include "framework/utility/timer.h"

#include "definitions/usm_random_memory_allocation.h"

#include <gtest/gtest.h>
#include <random>

static TestResult run(const UsmRandomMemoryAllocationArguments &arguments,
                      Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds,
                                   MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(),
                                   typeSelector.getType());
        return TestResult::Nooped;
    }

    OlState ol;
    Timer timer;

    auto size = makeRandomDistribution(arguments.sizeDistribution,
                                       arguments.minSize, arguments.maxSize);
    auto operation = std::uniform_int_distribution<int>{0, 1};

    std::vector<void *> ptrs;
    std::mt19937 gen{0};
    ptrs.reserve(arguments.operationCount * arguments.iterations);
    ptrs.resize(arguments.operationCount);
    for (auto &ptr : ptrs) {
        ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::allocate(
            arguments.usmMemoryPlacement, ol.context, ol.device, size->get(gen),
            &ptr));
    }

    for (auto j = 0u; j < arguments.iterations; j++) {
        timer.measureStart();

        if (operation(gen) || ptrs.empty()) {
            void *ptr;
            ASSERT_OL_RESULT_SUCCESS(OL::UsmHelper::allocate(
                arguments.usmMemoryPlacement, ol.context, ol.device,
                size->get(gen), &ptr));
            ptrs.push_back(ptr);
        } else {
            size_t idx =
                std::uniform_int_distribution<size_t>{0, ptrs.size() - 1}(gen);
            std::swap(ptrs[idx], ptrs.back());
            ASSERT_OL_RESULT_SUCCESS(olMemFree(ol.context, ptrs.back()));
            ptrs.pop_back();
        }

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(),
                             typeSelector.getType());
    }

    for (auto ptr : ptrs) {
        ASSERT_OL_RESULT_SUCCESS(olMemFree(ol.context, ptr));
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<
    UsmRandomMemoryAllocation>
    registerTestCase(run, Api::OL);
