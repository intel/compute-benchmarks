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

    const size_t patternSize = arguments.patternSize;
    const size_t dstOffset = arguments.dstOffset;
    const size_t size = arguments.size;
    if (patternSize == 0 || dstOffset % patternSize != 0 || size % patternSize != 0) {
        std::cerr << "Invalid arguments: dstOffset " << dstOffset << " and size " << size << " must be multiples of patternSize " << patternSize << std::endl;
        return TestResult::InvalidArgs;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    if (patternSize > levelzero.commandQueueMaxFillSize) {
        std::cerr << "Skipping: patternSize " << patternSize << " exceeds the maximum fill pattern size " << levelzero.commandQueueMaxFillSize << " of the selected queue" << std::endl;
        return TestResult::DeviceNotCapable;
    }

    const size_t dstAllocationSize = size + dstOffset;
    BigUsmAllocations allocations(levelzero);
    void *dstAllocation{};
    if (const auto result = allocations.allocate(arguments.dst, dstAllocationSize, &dstAllocation); result != TestResult::Success) {
        return result;
    }
    void *dst = static_cast<char *>(dstAllocation) + dstOffset;
    const std::vector<uint8_t> pattern(patternSize, 0xAB);

    return measureAppendMemory(levelzero, arguments, statistics, typeSelector, [&](ze_command_list_handle_t cmdList, ze_event_handle_t event) {
        return zeCommandListAppendMemoryFill(cmdList, dst, pattern.data(), patternSize, size, event, 0, nullptr);
    });
}

static RegisterTestCaseImplementation<AppendMemFill> registerTestCase(run, Api::L0);
