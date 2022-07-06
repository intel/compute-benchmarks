/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_for_copy_engine.h"

#include <gtest/gtest.h>

static TestResult verifyCopyEngineExists() {
    LevelZero levelzero;

    uint32_t numQueueGroups = 0;
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device,
                                                                    &numQueueGroups,
                                                                    nullptr));
    if (numQueueGroups == 0) {
        return TestResult::DeviceNotCapable;
    }

    std::vector<ze_command_queue_group_properties_t> queueProperties(numQueueGroups);
    ASSERT_ZE_RESULT_SUCCESS(zeDeviceGetCommandQueueGroupProperties(levelzero.device,
                                                                    &numQueueGroups,
                                                                    queueProperties.data()));

    for (uint32_t i = 0; i < numQueueGroups; i++) {
        if ((queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0 &&
            (queueProperties[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY)) {
            if (queueProperties[i].numQueues > 0) {
                return TestResult::Success;
            }
        }
    }
    return TestResult::DeviceNotCapable;
}

static TestResult run(const ExecuteCommandListArguments &arguments, Statistics &statistics) {

    // Verify copy queue exists
    if (auto ret = verifyCopyEngineExists(); ret != TestResult::Success) {
        return ret;
    }

    // Setup
    Timer timer;
    auto queueProperties = QueueProperties::create().setForceEngine(Engine::Bcs);
    auto levelzero = LevelZero{queueProperties};

    // Create fence if neccessary
    ze_fence_handle_t fence{};
    if (arguments.useFence) {
        const ze_fence_desc_t fenceDesc{ZE_STRUCTURE_TYPE_FENCE_DESC};
        ASSERT_ZE_RESULT_SUCCESS(zeFenceCreate(levelzero.commandQueue, &fenceDesc, &fence));
    }

    // Create buffers
    void *source{}, *destination{};
    size_t bufferSize = 4096;
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, bufferSize, &source));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, source, bufferSize));
    memset(static_cast<uint8_t *>(source), 0x7c, bufferSize);
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, bufferSize, &destination));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, destination, bufferSize));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, bufferSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, fence));
    if (arguments.useFence) {
        ASSERT_ZE_RESULT_SUCCESS(zeFenceHostSynchronize(fence, std::numeric_limits<uint64_t>::max()));
    } else {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        if (arguments.useFence) {
            ASSERT_ZE_RESULT_SUCCESS(zeFenceReset(fence));
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, fence));
        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
        }

        if (arguments.useFence) {
            ASSERT_ZE_RESULT_SUCCESS(zeFenceHostSynchronize(fence, std::numeric_limits<uint64_t>::max()));
        } else {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        }
        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
        }
    }

    // Release resources
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, source, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, source));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, destination, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, destination));
    if (arguments.useFence) {
        ASSERT_ZE_RESULT_SUCCESS(zeFenceDestroy(fence));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListForCopyEngine> registerTestCase(run, Api::L0);
