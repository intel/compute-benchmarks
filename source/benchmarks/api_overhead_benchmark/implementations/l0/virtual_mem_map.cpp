/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_map.h"

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include <gtest/gtest.h>

using namespace L0;

static size_t getPageAlignedSize(size_t requestedSize, size_t pageSize) {
    if (pageSize >= requestedSize) {
        return pageSize;
    } else {
        return (requestedSize / pageSize) * pageSize;
    }
}

static TestResult createPoolSlot(LevelZero &levelzero, const size_t mapSize, const size_t physicalSize,
                                 void *&virtualMem, ze_physical_mem_handle_t &physicalMemoryHandle) {

    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, mapSize, &virtualMem));
    EXPECT_NE(virtualMem, static_cast<void *>(nullptr));

    ze_physical_mem_desc_t physDesc = {ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC, nullptr};
    physDesc.size = physicalSize;
    ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemCreate(levelzero.context, levelzero.device, &physDesc, &physicalMemoryHandle));

    return TestResult::Success;
}

static TestResult destroyPoolSlot(LevelZero &levelzero, const size_t mapSize,
                                  void *virtualMem, ze_physical_mem_handle_t physicalMemoryHandle) {

    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemUnmap(levelzero.context, virtualMem, mapSize));
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, virtualMem, mapSize));
    ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemDestroy(levelzero.context, physicalMemoryHandle));

    return TestResult::Success;
}

static TestResult run(const VirtualMemMapArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    size_t pageSize = 0;
    size_t reserveSize = arguments.reserveSize;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.reserveSize, &pageSize));
    reserveSize = getPageAlignedSize(reserveSize, pageSize);

    const size_t mapSize = reserveSize;
    uint64_t alignedOffset = 0;
    size_t physicalSize = mapSize;
    if (arguments.useOffset != 0) {
        alignedOffset = pageSize;
        physicalSize = mapSize + pageSize;
    }

    ze_memory_access_attribute_t accessType = ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE;
    std::string accessTypeString = static_cast<const std::string &>(arguments.accessType);
    if (accessTypeString == "ReadOnly") {
        accessType = ZE_MEMORY_ACCESS_ATTRIBUTE_READONLY;
    } else if (accessTypeString == "None") {
        accessType = ZE_MEMORY_ACCESS_ATTRIBUTE_NONE;
    }

    // A recycled slot measures a remap, which is much cheaper than a first map, so every iteration (warmup
    // included) needs its own slot.
    const auto poolSize = static_cast<uint32_t>(arguments.poolSize);
    if (arguments.iterations > poolSize) {
        return TestResult::InvalidArgs;
    }

    const auto slotCount = static_cast<uint32_t>(arguments.iterations);
    std::vector<void *> reservedMemlist(slotCount, nullptr);
    std::vector<ze_physical_mem_handle_t> physicalMemoryHandleList(slotCount, nullptr);
    for (uint32_t slot = 0; slot < slotCount; slot++) {
        const auto status = createPoolSlot(levelzero, mapSize, physicalSize, reservedMemlist[slot], physicalMemoryHandleList[slot]);
        if (status != TestResult::Success) {
            return status;
        }
    }

    Timer timer;
    for (uint32_t slot = 0; slot < slotCount; slot++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemMap(levelzero.context, reservedMemlist[slot], mapSize,
                                                 physicalMemoryHandleList[slot], alignedOffset,
                                                 accessType));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (uint32_t slot = 0; slot < slotCount; slot++) {
        const auto status = destroyPoolSlot(levelzero, mapSize, reservedMemlist[slot], physicalMemoryHandleList[slot]);
        if (status != TestResult::Success) {
            return status;
        }
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemMap> registerTestCase(run, Api::L0);
