/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/submit_kernel.h"

#include <gtest/gtest.h>

static TestResult run(const SubmitKernelArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;
    const ze_group_count_t groupCount{1, 1, 1};

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("api_overhead_benchmark_eat_time.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "eat_time";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    const bool counterBasedEvents = arguments.inOrderQueue;
    zex_counter_based_event_desc_t counterBasedEventDesc{ZE_STRUCTURE_TYPE_COUNTER_BASED_EVENT_POOL_EXP_DESC};
    counterBasedEventDesc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE | ZEX_COUNTER_BASED_EVENT_FLAG_HOST_VISIBLE;
    counterBasedEventDesc.flags |= arguments.useProfiling ? ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP : 0;
    counterBasedEventDesc.signalScope = ZE_EVENT_SCOPE_FLAG_DEVICE;
    counterBasedEventDesc.waitScope = ZE_EVENT_SCOPE_FLAG_HOST;

    // Create event pool
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.flags |= arguments.useProfiling ? ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP : 0;
    eventPoolDesc.count = static_cast<uint32_t>(arguments.numKernels); // ensures one unique event per kernel

    ze_event_pool_handle_t eventPool = nullptr;
    if (!counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    }

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    std::vector<ze_event_handle_t> events(arguments.numKernels);

    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime);

    // Create an immediate command list
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.inOrderQueue) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Warmup
    for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
        // Note: this test calls zeKernelSetArgumentValue and zeKernelSetGroupSize each time to be closer to the SYCL behavior!
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
        // This isn't exactly what SYCL does, but it is a reasonable approximation.
        ze_event_handle_t signalEvent = nullptr;
        if (!arguments.discardEvents) {

            if (counterBasedEvents) {
                ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &events[iteration]));
            } else {
                eventDesc.index = iteration;
                ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[iteration]));
            }
            signalEvent = events[iteration];
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, signalEvent, 0, nullptr));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    if (!arguments.discardEvents) {
        for (auto event : events) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
        }
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (auto iteration = 0u; iteration < arguments.numKernels; iteration++) {
            // Note: this test calls zeKernelSetArgumentValue and zeKernelSetGroupSize each time to be closer to the SYCL behavior!
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
            ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
            // This isn't exactly what SYCL does, but it is a reasonable approximation.
            ze_event_handle_t signalEvent = nullptr;
            if (!arguments.discardEvents) {
                if (counterBasedEvents) {
                    ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &counterBasedEventDesc, &events[iteration]));
                } else {
                    eventDesc.index = iteration;
                    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[iteration]));
                }
                signalEvent = events[iteration];
            }
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, signalEvent, 0, nullptr));
        }

        if (!arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }

        if (!arguments.discardEvents) {
            for (auto event : events) {
                ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event));
            }
        }
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

    // Clean up
    if (!counterBasedEvents) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<SubmitKernel> registerTestCase(run, Api::L0);
