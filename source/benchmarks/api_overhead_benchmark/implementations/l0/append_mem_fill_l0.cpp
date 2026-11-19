/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/append_mem_fill.h"
#include "utility/l0/append_memory_utility_l0.h"

#include <gtest/gtest.h>
#include <vector>

using namespace L0;

static TestResult run(const AppendMemFillArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr || arguments.patternSize > levelzero.commandQueueMaxFillSize) {
        return TestResult::DeviceNotCapable;
    }

    const size_t dstAllocationSize = arguments.size + arguments.dstOffset;
    if (!BigUsmAllocations::fitsInDeviceMemory(levelzero, {{arguments.dst, dstAllocationSize}})) {
        return TestResult::DeviceNotCapable;
    }
    BigUsmAllocations allocations(levelzero);
    void *dstAllocation{};
    if (const auto result = allocations.allocate(arguments.dst, dstAllocationSize, &dstAllocation); result != TestResult::Success) {
        return result;
    }
    void *dst = static_cast<char *>(dstAllocation) + arguments.dstOffset;
    const std::vector<uint8_t> pattern(arguments.patternSize, 0xAB);

    return measureAppendMemory(levelzero, arguments, arguments.useEvent, arguments.appendCount, arguments.useImmediate, statistics, typeSelector,
                               [&](ze_command_list_handle_t cmdList, ze_event_handle_t event) {
                                   return zeCommandListAppendMemoryFill(cmdList, dst, pattern.data(), arguments.patternSize, arguments.size, event, 0, nullptr);
                               });
}

static RegisterTestCaseImplementation<AppendMemFill> registerTestCase(run, Api::L0);
