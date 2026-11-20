/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"

#include "definitions/append_mem_copy.h"
#include "utility/l0/append_memory_utility_l0.h"

#include <gtest/gtest.h>

using namespace L0;

static TestResult run(const AppendMemCopyArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    const size_t srcAllocationSize = arguments.size + arguments.srcOffset;
    const size_t dstAllocationSize = arguments.size + arguments.dstOffset;
    BigUsmAllocations allocations(levelzero);
    void *srcAllocation{};
    void *dstAllocation{};
    if (const auto result = allocations.allocate(arguments.src, srcAllocationSize, &srcAllocation); result != TestResult::Success) {
        return result;
    }
    if (const auto result = allocations.allocate(arguments.dst, dstAllocationSize, &dstAllocation); result != TestResult::Success) {
        return result;
    }
    const void *src = static_cast<const char *>(srcAllocation) + arguments.srcOffset;
    void *dst = static_cast<char *>(dstAllocation) + arguments.dstOffset;

    return measureAppendMemory(levelzero, arguments, statistics, typeSelector, [&](ze_command_list_handle_t cmdList, ze_event_handle_t event) {
        return zeCommandListAppendMemoryCopy(cmdList, dst, src, arguments.size, event, 0, nullptr);
    });
}

static RegisterTestCaseImplementation<AppendMemCopy> registerTestCase(run, Api::L0);
