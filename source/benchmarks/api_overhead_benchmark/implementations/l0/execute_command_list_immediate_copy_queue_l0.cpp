/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_immediate_copy_queue.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateCopyQueueArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    Timer timer;

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.isCopyOnly).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
                                                                               (requiresImport(arguments.sourcePlacement) ||
                                                                                requiresImport(arguments.destinationPlacement)))
                                                  .setCounterBasedCreateFunctions(arguments.useIoq);

    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }

    // Create event
    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useIoq) {
        zex_counter_based_event_exp_flags_t counterBasedDescFlags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
        const ze_event_scope_flags_t signalScope = ZE_EVENT_SCOPE_FLAG_HOST;
        const ze_event_scope_flags_t waitScope = 0;
        const zex_counter_based_event_desc_t counterBasedEventDesc = {.stype = ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC, .pNext = nullptr, .flags = counterBasedDescFlags, .signalScope = signalScope, .waitScope = waitScope};
        ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &event));
    } else {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.index = 0;
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    // Create buffers
    void *srcBuffer{}, *dstBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &srcBuffer));
    if (isUsmMemoryType(arguments.sourcePlacement)) {
        ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, srcBuffer, arguments.size, BufferContents::Zeros, true));
    } else {
        memset(srcBuffer, 0, arguments.size);
    }
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &dstBuffer));

    // Create an immediate command list
    ze_command_list_handle_t cmdList{};
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, queueProperties.selectedEngine);
    if (arguments.useIoq) {
        commandQueueDesc->desc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }

    zex_intel_queue_copy_operations_offload_hint_exp_desc_t copyOffload = {ZEX_INTEL_STRUCTURE_TYPE_QUEUE_COPY_OPERATIONS_OFFLOAD_HINT_EXP_PROPERTIES, nullptr, true};
    if (arguments.withCopyOffload) {
        commandQueueDesc->desc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
        commandQueueDesc->desc.pNext = &copyOffload;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dstBuffer, srcBuffer, arguments.size, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
    if (!arguments.useIoq) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, dstBuffer, srcBuffer, arguments.size, event, 0, nullptr));

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
        if (!arguments.useIoq) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        }
    }

    // Release
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    if (!arguments.useIoq) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, srcBuffer));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, dstBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListImmediateCopyQueue> registerTestCase(run, Api::L0);
