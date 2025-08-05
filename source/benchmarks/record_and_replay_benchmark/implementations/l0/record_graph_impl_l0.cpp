/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/random_distribution.h"
#include "framework/utility/timer.h"

#include "definitions/record_graph.h"

#include <gtest/gtest.h>
#include <level_zero/ze_api.h>
#include <list>

namespace {

} // namespace

static TestResult run(const RecordGraphArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    Timer timer;
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<RecordGraph> registerTestCase(run, Api::L0);
