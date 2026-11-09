/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/compiler_options_builder.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_bidirectional_kernel.h"

using namespace L0;

static TestResult run(const UsmCopyBidirectionalKernelArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create().allowCreationFail();
    LevelZero levelzero(queueProperties);
    if (levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    void *hostBuffer{}, *deviceBuffer{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.size, &hostBuffer));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.size, &deviceBuffer));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, hostBuffer, arguments.size, BufferContents::Random, false));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, deviceBuffer, arguments.size, BufferContents::Random, false));

    CompilerOptionsBuilder compilerOptions;
    compilerOptions.addDefinitionKeyValue("STRIPE_ELEMENTS", UsmCopyBidirectionalKernelArguments::stripeElements);
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = KernelHelper::loadKernel(levelzero, "memory_benchmark_usm_copy_bidirectional_kernel.cl", "copy_bidirectional_stripes", &kernel, &module, compilerOptions.str().c_str());
        result != TestResult::Success) {
        return result;
    }

    const uint32_t elementsCount = static_cast<uint32_t>(arguments.size / UsmCopyBidirectionalKernelArguments::elementSize);
    uint32_t groupSizeX{}, groupSizeY{}, groupSizeZ{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSuggestGroupSize(kernel, elementsCount, 1u, 1u, &groupSizeX, &groupSizeY, &groupSizeZ));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, groupSizeY, groupSizeZ));
    const ze_group_count_t groupCount{elementsCount / groupSizeX, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(hostBuffer), &hostBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(deviceBuffer), &deviceBuffer));

    ze_event_pool_handle_t eventPool{};
    ze_event_handle_t event{};
    if (arguments.useEvents) {
        ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
        eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
        eventPoolDesc.count = 1;
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
        ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
        eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
        eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
    }

    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t timestampResult{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(event, &timestampResult));
            const auto commandTime = levelzero.getAbsoluteKernelExecutionTime(timestampResult.global);
            statistics.pushValue(commandTime, arguments.size, typeSelector.getUnit(), typeSelector.getType());
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(event));
        } else {
            statistics.pushValue(timer.get(), arguments.size, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    if (arguments.useEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, hostBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, deviceBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyBidirectionalKernel> registerTestCase(run, Api::L0);
