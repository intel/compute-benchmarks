/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/append_memory_copy_from_context.h"

#include <vector>

using namespace L0;

static TestResult run(const AppendMemoryCopyFromContextArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    const UsmMemoryPlacement dstPlacement = arguments.dst;
    if (!isUsmMemoryType(dstPlacement) && !isSharedSystemPointer(dstPlacement)) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().setForceBlitter(arguments.forceBlitter).allowCreationFail();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(requiresImport(dstPlacement));
    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    const ze_context_desc_t contextDesc{ZE_STRUCTURE_TYPE_CONTEXT_DESC};
    ze_context_handle_t srcContext{};
    ASSERT_ZE_RESULT_SUCCESS(zeContextCreate(levelzero.driver, &contextDesc, &srcContext));

    void *src{};
    void *dst{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.src, srcContext, levelzero.device, arguments.size, &src));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.dst, levelzero, arguments.size, &dst));

    const uint32_t appendCount = static_cast<uint32_t>(arguments.appendCount);
    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, 0, appendCount};
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_DEVICE};
    ze_event_pool_handle_t eventPool{};
    std::vector<ze_event_handle_t> events(appendCount);
    if (arguments.useEvent) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 0, nullptr, &eventPool));
        for (auto j = 0u; j < appendCount; ++j) {
            eventDesc.index = j;
            ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[j]));
        }
    }

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));

    for (auto i = 0u; i < arguments.iterations; ++i) {
        timer.measureStart();
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopyFromContext(cmdList, dst, srcContext, src, arguments.size, events[j], 0, nullptr));
        }
        timer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(cmdList));
        statistics.pushValue(timer.get() / appendCount, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvent) {
        for (auto j = 0u; j < appendCount; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[j]));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.dst, levelzero, dst));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(srcContext, src));
    ASSERT_ZE_RESULT_SUCCESS(zeContextDestroy(srcContext));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<AppendMemoryCopyFromContext> registerTestCase(run, Api::L0);
