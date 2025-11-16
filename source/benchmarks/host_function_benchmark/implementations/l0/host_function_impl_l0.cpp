/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/host_function.h"
#include "utility/l0/host_function_utility_l0.h"

#include <level_zero/ze_api.h>

static TestResult run(const HostFunctionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timerWithHostFunction;
    Timer timerWithoutHostFunction;

    HostFunctionApi hostFunctionApi = loadHostFunctionApi(levelzero.driver);
    HostFunctions hostFunctions = getHostFunctions();

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

    // Create an immediate command list
    const ze_group_count_t groupCount{1, 1, 1};
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    if (arguments.useIoq) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));

    if (arguments.useHostFunctionColdRun == false) {
        ASSERT_ZE_RESULT_SUCCESS(hostFunctionApi.commandListAppendHostFunction(cmdList,
                                                                               hostFunctions.emptyHostFunction,
                                                                               hostFunctions.emptyHostFunctionUserData,
                                                                               nullptr,
                                                                               nullptr,
                                                                               0,
                                                                               nullptr));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    auto limit = arguments.amountOfCalls;

    for (auto i = 0u; i < arguments.iterations; i++) {

        timerWithHostFunction.measureStart();
        for (uint32_t callId = 0u; callId < limit; callId++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(hostFunctionApi.commandListAppendHostFunction(cmdList,
                                                                                   hostFunctions.emptyHostFunction,
                                                                                   hostFunctions.emptyHostFunctionUserData,
                                                                                   nullptr,
                                                                                   nullptr,
                                                                                   0,
                                                                                   nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }
        if (arguments.measureCompletionTime) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        }
        timerWithHostFunction.measureEnd();

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));

        timerWithoutHostFunction.measureStart();
        for (uint32_t callId = 0u; callId < limit; callId++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }
        if (arguments.measureCompletionTime) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        }
        timerWithoutHostFunction.measureEnd();

        auto duration = timerWithHostFunction.get() - timerWithoutHostFunction.get();
        statistics.pushValue(duration, typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<HostFunction> registerTestCase(run, Api::L0, true);
