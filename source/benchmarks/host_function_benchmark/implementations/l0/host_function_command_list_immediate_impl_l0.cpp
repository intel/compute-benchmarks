/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/host_function_command_list_immediate.h"
#include "utility/l0/host_function_utility_l0.h"

#include <level_zero/ze_api.h>
#include <level_zero/zer_api.h>

static TestResult run(const HostFunctionCommandListImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Nanoseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    bool useKernels = arguments.useKernels;
    ExtensionProperties extensionProperties = ExtensionProperties::create();
    extensionProperties.setCounterBasedCreateFunctions(useKernels);
    extensionProperties.setHostFunctionFunctions(true);
    LevelZero levelzero(extensionProperties);

    Timer timer;
    auto &nCalls = arguments.amountOfCalls;

    HostFunctions hostFunctions = getHostFunctions(arguments.useEmptyHostFunction);

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

    // Configure kernel
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.kernelExecutionTime * 4);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));

    zex_counter_based_event_desc_t counterBasedEventDesc{ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC};
    counterBasedEventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;

    std::vector<ze_event_handle_t> eventKernel1;
    std::vector<ze_event_handle_t> eventKernel2;

    if (useKernels) {
        eventKernel1.resize(nCalls);
        eventKernel2.resize(nCalls);

        for (auto &event : eventKernel1) {
            ASSERT_ZE_RESULT_SUCCESS(
                levelzero.counterBasedEventCreate2(levelzero.context,
                                                   levelzero.device,
                                                   &counterBasedEventDesc,
                                                   &event));
        }
        for (auto &event : eventKernel2) {
            ASSERT_ZE_RESULT_SUCCESS(
                levelzero.counterBasedEventCreate2(levelzero.context,
                                                   levelzero.device,
                                                   &counterBasedEventDesc,
                                                   &event));
        }
    }

    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdList));

    // Warmup
    const ze_group_count_t groupCount{1, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(levelzero.commandListAppendHostFunction(cmdList,
                                                                     hostFunctions.function,
                                                                     hostFunctions.userData,
                                                                     nullptr,
                                                                     nullptr,
                                                                     0,
                                                                     nullptr));

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

    // Benchmark

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();

        for (uint32_t j = 0u; j < nCalls; j++) {

            if (useKernels) {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, eventKernel1[j], 0, nullptr));
            }

            ASSERT_ZE_RESULT_SUCCESS(levelzero.commandListAppendHostFunction(cmdList,
                                                                             hostFunctions.function,
                                                                             hostFunctions.userData,
                                                                             nullptr,
                                                                             nullptr,
                                                                             0,
                                                                             nullptr));

            if (useKernels) {
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, eventKernel2[j], 1, &eventKernel1[j]));
            }
        }

        if (arguments.measureCompletionTime == false) {
            timer.measureEnd();
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

        if (arguments.measureCompletionTime) {
            timer.measureEnd();
        }

        if (useKernels) {
            std::chrono::nanoseconds duration{0};
            for (auto j = 0u; j < nCalls; j++) {

                auto hostFunctionDuration = levelzero.getAbsoluteTimeBetweenTwoKernels(eventKernel1[j], eventKernel2[j]);
                if (hostFunctionDuration.has_value() == false) {
                    return TestResult::Error;
                }
                duration += hostFunctionDuration.value();
            }

            duration /= nCalls;
            statistics.pushValue(duration, typeSelector.getUnit(), typeSelector.getType());
        } else {
            auto duration = timer.get() / nCalls;
            statistics.pushValue(duration, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Cleanup
    for (auto &event : eventKernel1) {
        zeEventDestroy(event);
    }
    for (auto &event : eventKernel2) {
        zeEventDestroy(event);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<HostFunctionCommandListImmediate> registerTestCase(run, Api::L0, true);
