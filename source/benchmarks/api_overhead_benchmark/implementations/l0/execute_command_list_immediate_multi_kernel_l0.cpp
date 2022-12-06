/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/execute_command_list_immediate_multi_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const ExecuteCommandListImmediateMultiKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;

    // Create kernel
    auto spirvModule0 = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule0.size() == 0) {
        return TestResult::KernelNotFound;
    }
    auto spirvModule1 = FileHelper::loadBinaryFile("api_overhead_benchmark_write_one.spv");
    if (spirvModule1.size() == 0) {
        return TestResult::KernelNotFound;
    }
    uint32_t moduleCount = 2u;
    std::vector<ze_module_handle_t> modules(moduleCount);
    std::vector<ze_kernel_handle_t> kernels(moduleCount);
    std::vector<ze_module_desc_t> moduleDescs(moduleCount);
    std::vector<ze_kernel_desc_t> kernelDescs(moduleCount);
    moduleDescs[0].format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDescs[0].pInputModule = reinterpret_cast<const uint8_t *>(spirvModule0.data());
    moduleDescs[0].inputSize = spirvModule0.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDescs[0], &modules[0], nullptr));
    kernelDescs[0].pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(modules[0], &kernelDescs[0], &kernels[0]));

    moduleDescs[1].format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDescs[1].pInputModule = reinterpret_cast<const uint8_t *>(spirvModule1.data());
    moduleDescs[1].inputSize = spirvModule1.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDescs[1], &modules[1], nullptr));
    kernelDescs[1].pKernelName = "write_one";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(modules[1], &kernelDescs[1], &kernels[1]));

    // Create event
    uint32_t numEventsMultiplier = arguments.addBarrier ? static_cast<uint32_t>(arguments.numKernelsAfterBarrier + 1) : 2u;
    uint32_t eventsCount = static_cast<uint32_t>(arguments.amountOfCalls) * numEventsMultiplier;
    ze_event_pool_desc_t eventPoolDesc;
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = eventsCount;
    ze_event_pool_handle_t eventPool{};
    ze_event_desc_t eventDesc = {ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_DEVICE, ZE_EVENT_SCOPE_FLAG_HOST};
    std::vector<ze_event_handle_t> events(eventsCount);
    ZE_RESULT_SUCCESS_OR_ERROR(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    for (uint32_t eventIndex = 0u; eventIndex < eventsCount; eventIndex++) {
        eventDesc.index = eventIndex;
        auto event = &events[eventIndex];
        ZE_RESULT_SUCCESS_OR_ERROR(zeEventCreate(eventPool, &eventDesc, event));
    }

    // Create output buffer
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *outputBuffer = nullptr;
    const auto outputBufferSize = sizeof(uint32_t) * 1000 * 256;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, outputBufferSize, 0, levelzero.device, &outputBuffer));

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[0], 1u, 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime * 4);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[0], 0, sizeof(int), &kernelOperationsCount));
    const ze_group_count_t groupCount0{1, 1, 1};

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[1], 256u, 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[1], 0, sizeof(outputBuffer), &outputBuffer));

    // Create an immediate command list
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[0], &groupCount0, events[0], 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[1], &groupCount0, events[1], 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[0], std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[1], std::numeric_limits<uint64_t>::max()));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[0]));
    ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[1]));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        uint32_t eventId = 0u;
        timer.measureStart();
        for (uint32_t callId = 0u; callId < arguments.amountOfCalls; callId++) {
            if (arguments.addBarrier) {
                uint32_t numKernelsToAdd = (((arguments.numKernelsBeforeBarrier) % 2) == 0) ? static_cast<uint32_t>(arguments.numKernelsBeforeBarrier) : static_cast<uint32_t>(arguments.numKernelsBeforeBarrier) + 1;
                for (uint32_t kernelId = 0; kernelId < numKernelsToAdd; kernelId++) {
                    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[kernelId % 2], &groupCount0, nullptr, 0, nullptr));
                }

                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendBarrier(cmdList, nullptr, 0, nullptr));

                numKernelsToAdd = (((arguments.numKernelsAfterBarrier) % 2) == 0) ? static_cast<uint32_t>(arguments.numKernelsAfterBarrier) : static_cast<uint32_t>(arguments.numKernelsAfterBarrier + 1);
                for (uint32_t kernelId = 0; kernelId < numKernelsToAdd; kernelId++) {
                    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[kernelId % 2], &groupCount0,
                                                                             events[eventId++], 0, nullptr));
                }
            } else {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[0], &groupCount0,
                                                                         arguments.addBarrier ? nullptr : events[eventId++], 0, nullptr));
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[1], &groupCount0,
                                                                         arguments.addBarrier ? nullptr : events[eventId++], 0, nullptr));
            }
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        for (auto eventUsed = 0u; eventUsed < eventId; eventUsed++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[eventUsed], std::numeric_limits<uint64_t>::max()));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[eventUsed]));
        }
    }

    // Teardown
    for (auto &event : events) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    for (auto &kernel : kernels) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    }
    for (auto &module : modules) {
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<ExecuteCommandListImmediateMultiKernel> registerTestCase(run, Api::L0);
