/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/memory_constants.h"
#include "framework/utility/timer.h"

#include "definitions/mem_open_ipc_handle.h"

#include <gtest/gtest.h>

static TestResult run(const MemOpenIpcHandleArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    if ((levelzero.getIpcProperties().flags & ZE_IPC_PROPERTY_FLAG_MEMORY) == 0) {
        return TestResult::DeviceNotCapable;
    }

    // Create buffer
    std::vector<void *> allocations;
    allocations.reserve(arguments.AllocationsCount);

    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        auto bufferSize = sizeof(int32_t);
        void *ptr = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, bufferSize, &ptr));
        allocations.push_back(ptr);
    }

    auto modifyAllocations = [&]() {
        for (int64_t index = 0; index < arguments.AllocationsCount; index++) {
            // Modify alternate allocations
            if (index & 1) {
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, allocations[index]));
                auto bufferSize = sizeof(uint64_t);
                void *ptr = nullptr;
                ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, bufferSize, &ptr));
                allocations[index] = ptr;
            }
        }
        return TestResult::Success;
    };

    // Warmup
    std::vector<ze_ipc_mem_handle_t> ipcHandles;
    ipcHandles.reserve(arguments.AllocationsCount);
    std::vector<void *> ipcPointers(arguments.AllocationsCount);
    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        std::fill_n(ipcHandles[i].data, ZE_MAX_IPC_HANDLE_SIZE, static_cast<char>(0));
        ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(levelzero.context, allocations[i], &ipcHandles[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeMemOpenIpcHandle(levelzero.context, levelzero.device, ipcHandles[i], 0, &ipcPointers[i]));
        EXPECT_NE(ipcPointers[i], nullptr);
    }
    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(levelzero.context, ipcHandles[i]));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        for (int64_t j = 0; j < arguments.AllocationsCount; ++j) {
            std::fill_n(ipcHandles[j].data, ZE_MAX_IPC_HANDLE_SIZE, static_cast<char>(0));
            ASSERT_ZE_RESULT_SUCCESS(zeMemGetIpcHandle(levelzero.context, allocations[j], &ipcHandles[j]));
        }

        timer.measureStart();
        for (int64_t j = 0; j < arguments.AllocationsCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemOpenIpcHandle(levelzero.context, levelzero.device, ipcHandles[j], 0, &ipcPointers[j]));
        }
        timer.measureEnd();

        for (int64_t j = 0; j < arguments.AllocationsCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemPutIpcHandle(levelzero.context, ipcHandles[j]));
            ipcPointers[j] = nullptr;
        }

        auto status = modifyAllocations();
        if (status != TestResult::Success) {
            return status;
        }

        statistics.pushValue(timer.get() / arguments.AllocationsCount, typeSelector.getUnit(), typeSelector.getType());
    }

    // Cleanup
    for (int64_t i = 0; i < arguments.AllocationsCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, allocations[i]));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<MemOpenIpcHandle> registerTestCase(run, Api::L0);