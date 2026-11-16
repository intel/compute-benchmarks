/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/l0/utility/error.h"
#include "framework/test_case/test_result.h"
#include "framework/utility/statistics.h"
#include "framework/utility/timer.h"

#include <array>
#include <level_zero/ze_api.h>

inline constexpr uint32_t kernelSetArgumentValueArgumentCount = 8u;
inline constexpr uint32_t kernelSetArgumentValueCallsPerSample = 100u;

template <typename ArgumentValue>
TestResult measureKernelSetArgumentValue(ze_kernel_handle_t kernel, uint32_t argumentIndex, const std::array<ArgumentValue, 2> &values,
                                         bool differentValues, const size_t &iterations, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);
    Timer timer;
    for (auto i = 0u; i < iterations; i++) {
        timer.measureStart();
        for (auto call = 0u; call < kernelSetArgumentValueCallsPerSample; call++) {
            const ArgumentValue &value = values[differentValues ? call % values.size() : 0u];
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, argumentIndex, sizeof(ArgumentValue), &value));
        }
        timer.measureEnd();
        statistics.pushFractionalValue(Statistics::FractionalDuration(timer.get()) / kernelSetArgumentValueCallsPerSample, typeSelector.getUnit(), typeSelector.getType());
    }
    return TestResult::Success;
}
