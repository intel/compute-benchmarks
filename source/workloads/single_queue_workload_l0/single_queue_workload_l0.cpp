/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/utility/timer.h"
#include "framework/workload/register_workload.h"

struct SingleQueueWorkloadArguments : WorkloadArgumentContainer {
    PositiveIntegerArgument operationsCount;
    PositiveIntegerArgument workgroupCount;
    PositiveIntegerArgument workgroupSize;

    SingleQueueWorkloadArguments()
        : operationsCount(*this, "operationsCount", "Number of redundant operations performed in kernel to make it take longer"),
          workgroupCount(*this, "wgc", "Number of workgroups enqueued"),
          workgroupSize(*this, "wgs", "Size of workgroups enqueued") {}
};

struct SingleQueueWorkload : Workload<SingleQueueWorkloadArguments> {};

TestResult run(const SingleQueueWorkloadArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {
    LevelZero levelzero{};
    Timer timer{};

    uint32_t tilesCount = {};
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeDeviceGetSubDevices(levelzero.device, &tilesCount, nullptr));
    if (tilesCount > 1) {
        io.writeToConsole("This workload should run on a single tile\n");
        return TestResult::DeviceNotCapable;
    }

    const auto totalThreadsCount = arguments.workgroupSize * arguments.workgroupCount;
    const ze_group_count_t groupCount{static_cast<uint32_t>(arguments.workgroupCount), 1, 1};
    const auto operationsCount = static_cast<uint32_t>(arguments.operationsCount);
    const auto bufferSizeInBytes = totalThreadsCount * sizeof(uint32_t);

    void *buffer = nullptr;
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSizeInBytes, 0, levelzero.device, &buffer));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSizeInBytes));

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "single_queue_workload_increment.cl", "increment", &kernel, &module, nullptr);
        result != TestResult::Success) {
        return result;
    }
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(arguments.workgroupSize), 1u, 1u));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelSetArgumentValue(kernel, 1, sizeof(operationsCount), &operationsCount));

    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListClose(cmdList));

    for (auto i = 0u; i < arguments.iterations; i++) {
        synchronization.synchronize(io);

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), MeasurementUnit::Unknown, MeasurementType::Unknown);
    }

    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSizeInBytes));

    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListDestroy(cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelDestroy(kernel));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeModuleDestroy(module));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

int main(int argc, char **argv) {
    SingleQueueWorkload workload;
    SingleQueueWorkload::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
