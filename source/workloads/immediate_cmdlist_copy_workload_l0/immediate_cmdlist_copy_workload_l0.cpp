/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"
#include "framework/workload/register_workload.h"

struct ImmediateCmdListCopyWorkloadArguments : WorkloadArgumentContainer {
    Uint32Argument ordinal;
    Uint32Argument engineIndex;
    PositiveIntegerArgument copySize;

    ImmediateCmdListCopyWorkloadArguments()
        : ordinal(*this, "ordinal", "ordinal of engine group to be used"),
          engineIndex(*this, "engineIndex", "physical engine index inside the engine group"),
          copySize(*this, "copySize", "size in bytes to be used for copy operation") {}
};

struct ImmediateCmdListCopyWorkload : Workload<ImmediateCmdListCopyWorkloadArguments> {};

TestResult run(const ImmediateCmdListCopyWorkloadArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {

    // Setup
    LevelZero levelzero{};
    uint32_t queueGroupPropertiesCount = 0;
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &queueGroupPropertiesCount, nullptr));
    FATAL_ERROR_IF(queueGroupPropertiesCount == 0, "No queue group properties found!");
    std::vector<ze_command_queue_group_properties_t> queueGroupProperties(queueGroupPropertiesCount);
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeDeviceGetCommandQueueGroupProperties(levelzero.device, &queueGroupPropertiesCount, queueGroupProperties.data()));

    if (arguments.ordinal >= queueGroupPropertiesCount) {
        return TestResult::InvalidArgs;
    }

    if (arguments.engineIndex >= queueGroupProperties[arguments.ordinal].numQueues) {
        return TestResult::InvalidArgs;
    }

    ze_command_queue_desc_t commandQueueDesc = {};
    commandQueueDesc.stype = ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC;
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    commandQueueDesc.ordinal = static_cast<uint32_t>(arguments.ordinal);
    commandQueueDesc.index = static_cast<uint32_t>(arguments.engineIndex);

    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = 1;
    ze_event_pool_handle_t eventPool{};
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    ze_command_list_handle_t cmdList{};
    ze_event_handle_t event{};
    void *hostSrcMemory{};
    void *deviceDstMemory{};
    Timer timer{};

    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));
    eventDesc.index = 0;
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventCreate(eventPool, &eventDesc, &event));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, arguments.copySize, &hostSrcMemory));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(UsmHelper::allocate(UsmMemoryPlacement::Device, levelzero, arguments.copySize, &deviceDstMemory));

    // Warmup
    for (auto i = 0u; i < 5; i++) {
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListAppendMemoryCopy(cmdList, deviceDstMemory, hostSrcMemory, arguments.copySize,
                                                                        event, 0, nullptr));
        zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max());
        zeEventHostReset(event);
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        synchronization.synchronize(io);
        timer.measureStart();
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListAppendMemoryCopy(cmdList, deviceDstMemory, hostSrcMemory, arguments.copySize,
                                                                        event, 0, nullptr));
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventHostReset(event));
        statistics.pushValue(timer.get(), MeasurementUnit::Unknown, MeasurementType::Unknown);
    }
    // Cleanup
    UsmHelper::deallocate(UsmMemoryPlacement::Host, levelzero, hostSrcMemory);
    UsmHelper::deallocate(UsmMemoryPlacement::Device, levelzero, deviceDstMemory);
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventDestroy(event));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventPoolDestroy(eventPool));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListDestroy(cmdList));
    return TestResult::Success;
}

int main(int argc, char **argv) {
    ImmediateCmdListCopyWorkload workload;
    ImmediateCmdListCopyWorkload::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
