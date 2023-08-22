/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/virtual_mem_unmap.h"

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

static TestResult prepareVirtualMemoryMaps(LevelZero &levelzero, const size_t reserveSize,
                                           std::vector<void *> &reservedMemlist,
                                           std::vector<ze_physical_mem_handle_t> &physicalMemoryHandleList,
                                           const uint32_t iterationCount) {

    reservedMemlist.clear();
    physicalMemoryHandleList.clear();

    for (uint32_t i = 0; i < iterationCount; i++) {
        void *virtualMem = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemReserve(levelzero.context, nullptr, reserveSize, &virtualMem));
        EXPECT_NE(virtualMem, static_cast<void *>(nullptr));
        reservedMemlist.push_back(virtualMem);

        // Allocate Physical Memory handle
        ze_physical_mem_desc_t physDesc = {ZE_STRUCTURE_TYPE_PHYSICAL_MEM_DESC, nullptr};
        physDesc.size = reserveSize;
        ze_physical_mem_handle_t physicalMemoryHandle{};
        ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemCreate(levelzero.context, levelzero.device, &physDesc, &physicalMemoryHandle));
        physicalMemoryHandleList.push_back(physicalMemoryHandle);
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemMap(levelzero.context, virtualMem, reserveSize, physicalMemoryHandle, 0,
                                                 ZE_MEMORY_ACCESS_ATTRIBUTE_READWRITE));
    }

    return TestResult::Success;
}

static TestResult cleanupVirtualMemoryMaps(LevelZero &levelzero,
                                           std::vector<void *> &reservedMemlist,
                                           std::vector<ze_physical_mem_handle_t> &physicalMemoryHandleList,
                                           const size_t reserveSize) {

    for (auto &virtualMem : reservedMemlist) {
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemFree(levelzero.context, virtualMem, reserveSize));
    }

    for (auto &physicalMemoryHandle : physicalMemoryHandleList) {
        ASSERT_ZE_RESULT_SUCCESS(zePhysicalMemDestroy(levelzero.context, physicalMemoryHandle));
    }

    reservedMemlist.clear();
    physicalMemoryHandleList.clear();
    return TestResult::Success;
}

static TestResult run(const VirtualMemUnMapArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    size_t pageSize = 0;
    size_t reserveSize = arguments.reserveSize;
    ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemQueryPageSize(levelzero.context, levelzero.device,
                                                       arguments.reserveSize, &pageSize));
    reserveSize = getPageAlignedSize(reserveSize, pageSize);

    std::vector<void *> reservedMemlist{};
    std::vector<ze_physical_mem_handle_t> physicalMemoryHandleList{};

    // Warmup
    const uint32_t warmupIterationCount = 5;
    auto status = prepareVirtualMemoryMaps(levelzero, reserveSize, reservedMemlist, physicalMemoryHandleList, warmupIterationCount);
    if (status != TestResult::Success) {
        return status;
    }
    for (auto i = 0u; i < warmupIterationCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemUnmap(levelzero.context, reservedMemlist[i], reserveSize));
    }

    status = cleanupVirtualMemoryMaps(levelzero, reservedMemlist, physicalMemoryHandleList, reserveSize);
    if (status != TestResult::Success) {
        return status;
    }

    Timer timer;

    // Benchmark
    status = prepareVirtualMemoryMaps(levelzero, reserveSize, reservedMemlist, physicalMemoryHandleList, arguments.iterations);
    if (status != TestResult::Success) {
        return status;
    }

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeVirtualMemUnmap(levelzero.context, reservedMemlist[i], reserveSize));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    status = cleanupVirtualMemoryMaps(levelzero, reservedMemlist, physicalMemoryHandleList, reserveSize);
    if (status != TestResult::Success) {
        return status;
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<VirtualMemUnMap> registerTestCase(run, Api::L0);
