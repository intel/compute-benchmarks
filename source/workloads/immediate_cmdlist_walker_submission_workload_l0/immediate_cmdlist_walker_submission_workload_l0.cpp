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

#include <chrono>
#include <thread>

struct ImmediateCmdListWalkerSubmissionArguments : WorkloadArgumentContainer {
    Uint32Argument ordinal;
    Uint32Argument engineIndex;

    ImmediateCmdListWalkerSubmissionArguments()
        : ordinal(*this, "ordinal", "ordinal of engine group to be used"),
          engineIndex(*this, "engineIndex", "physical engine index inside the engine group") {}
};

struct ImmediateCmdListWalkerSubmission : Workload<ImmediateCmdListWalkerSubmissionArguments> {};

TestResult run(const ImmediateCmdListWalkerSubmissionArguments &arguments, Statistics &statistics, WorkloadSynchronization &synchronization, WorkloadIo &io) {

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
    Timer timer{};

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("ulls_benchmark_write_one.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "write_one";
    void *hostMemory;
    ze_kernel_handle_t kernel{};

    const size_t bufferSize = 4096;
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(UsmHelper::allocate(UsmMemoryPlacement::Host, levelzero, bufferSize, &hostMemory));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeContextMakeMemoryResident(levelzero.context, levelzero.device, hostMemory, bufferSize));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelCreate(module, &kernelDesc, &kernel));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelSetGroupSize(kernel, 1, 1, 1));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelSetArgumentValue(kernel, 0, sizeof(hostMemory), &hostMemory));
    eventDesc.index = 0;
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventCreate(eventPool, &eventDesc, &event));

    // Warmup
    const ze_group_count_t groupCount{1, 1, 1};
    for (auto i = 0u; i < 5; i++) {
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event, 0, nullptr));
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventHostReset(event));
    }
    volatile uint64_t *volatileBuffer = static_cast<uint64_t *>(hostMemory);
    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {

        *volatileBuffer = 0;
        _mm_clflush(hostMemory);
        synchronization.synchronize(io);
        timer.measureStart();
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, event, 0, nullptr));
        while (*volatileBuffer != 1) {
        }
        timer.measureEnd();
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventHostSynchronize(event, std::numeric_limits<uint64_t>::max()));
        ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventHostReset(event));
        zeCommandListReset(cmdList);
        statistics.pushValue(timer.get(), MeasurementUnit::Unknown, MeasurementType::Unknown);
    }

    // Cleanup
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeContextEvictMemory(levelzero.context, levelzero.device, hostMemory, bufferSize));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeKernelDestroy(kernel));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeMemFree(levelzero.context, hostMemory));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeCommandListDestroy(cmdList));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventDestroy(event));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeModuleDestroy(module));
    ZE_RESULT_SUCCESS_OR_RETURN_ERROR(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

int main(int argc, char **argv) {
    ImmediateCmdListWalkerSubmission workload;
    ImmediateCmdListWalkerSubmission::implementation = run;
    return workload.runFromCommandLine(argc, argv);
}
