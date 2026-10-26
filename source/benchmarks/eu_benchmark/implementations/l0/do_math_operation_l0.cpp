/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/math_operation_helper.h"
#include "framework/utility/timer.h"

#include "definitions/do_math_operation.h"
#include "kernel_helper.h"

#include <cstring>
#include <limits>
#include <vector>

using namespace L0;

static TestResult run(const DoMathOperationArguments &arguments, Statistics &statistics) {
    const MeasurementFields typeSelector(MeasurementUnit::Microseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    if (!MathOperationHelper::isSupportedAsNormal(arguments.operation, arguments.dataType)) {
        return TestResult::DeviceNotCapable;
    }
    if (DataTypeHelper::isIntegerType(arguments.dataType) && MathOperationHelper::getArgumentsCount(arguments.operation) == 1) {
        return TestResult::NoImplementation;
    }
    const OperationChain chain = ::KernelHelper::getOperationChain(arguments.dataType, arguments.operation);

    LevelZero levelzero;
    Timer timer{};

    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const size_t loopIterations = 200u;
    const size_t operationsPerLoop = 128u; // non-tweakable constant, this is hardcoded in the kernel code
    const size_t initIterations = arguments.mixGrfModes;
    const auto data = MathOperationHelper::generateTestData(arguments.dataType, arguments.operation, loopIterations, operationsPerLoop, 0);
    const uint32_t kernelLoopIterations = static_cast<uint32_t>(data.loopIterations);

    const char *programName = "eu_benchmark_perform_math_operation.cl";
    const char *kernelName = "do_math_operation";
    const std::string compilerOptions = ::KernelHelper::getCompilerOptions(arguments.dataType, arguments.operation, chain);
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, programName, kernelName, &kernel, &module, compilerOptions.c_str()); result != TestResult::Success) {
        return result;
    }

    ze_module_handle_t largeGrfModule{};
    ze_kernel_handle_t largeGrfKernel{};
    if (arguments.mixGrfModes) {
        const std::string largeGrfCompilerOptions = compilerOptions + " -ze-opt-large-register-file";
        if (auto result = L0::KernelHelper::loadKernel(levelzero, programName, kernelName, &largeGrfKernel, &largeGrfModule, largeGrfCompilerOptions.c_str()); result != TestResult::Success) {
            ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
            ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
            return result;
        }
    }

    const ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC, nullptr, ZE_EVENT_POOL_FLAG_KERNEL_TIMESTAMP | ZE_EVENT_POOL_FLAG_HOST_VISIBLE, 1};
    ze_event_pool_handle_t eventPool{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    const ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC, nullptr, 0, ZE_EVENT_SCOPE_FLAG_HOST, ZE_EVENT_SCOPE_FLAG_HOST};
    ze_event_handle_t profilingEvent{};
    ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &profilingEvent));
    const ze_event_handle_t eventForEnqueue = arguments.useEvents ? profilingEvent : nullptr;

    void *buffer = nullptr;
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, gws * data.sizeOfDataType, 0, levelzero.device, &buffer));

    std::vector<std::byte> initialData(gws * data.sizeOfDataType, std::byte{0});
    for (size_t i = 0; i < gws; i++) {
        std::memcpy(initialData.data() + i * data.sizeOfDataType, data.initialValue, data.sizeOfDataType);
    }
    ze_command_list_desc_t cmdListDesc{ZE_STRUCTURE_TYPE_COMMAND_LIST_DESC};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t copyCmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &copyCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(copyCmdList, buffer, initialData.data(), initialData.size(), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(copyCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &copyCmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};
    ze_command_list_handle_t largeGrfCmdList{};
    if (arguments.mixGrfModes) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(largeGrfKernel, static_cast<uint32_t>(lws), 1u, 1u));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(largeGrfKernel, 0, sizeof(buffer), &buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(largeGrfKernel, 1, data.sizeOfDataType, data.otherArgument));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(largeGrfKernel, 2, sizeof(kernelLoopIterations), &kernelLoopIterations));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &largeGrfCmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(largeGrfCmdList, largeGrfKernel, &groupCount, eventForEnqueue, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(largeGrfCmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &largeGrfCmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        if (arguments.useEvents) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvent));
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, data.sizeOfDataType, data.otherArgument));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(kernelLoopIterations), &kernelLoopIterations));
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, eventForEnqueue, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    for (auto i = 0u; i < arguments.iterations; i++) {
        ze_command_list_handle_t cmdListForExecution = cmdList;
        if (arguments.mixGrfModes && i % 2 == 0) {
            cmdListForExecution = largeGrfCmdList;
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdListForExecution, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t kernelTimestamp{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(profilingEvent, &kernelTimestamp));
            const auto timeNs = levelzero.getAbsoluteKernelExecutionTime(kernelTimestamp.global);
            statistics.pushValue(timeNs, typeSelector.getUnit(), typeSelector.getType());
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(profilingEvent));
        } else {
            statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
        }
    }

    const auto expected = ::KernelHelper::getExpectedResult(arguments, chain, loopIterations, operationsPerLoop, arguments.iterations + initIterations);
    auto testResult = TestResult::Success;
    std::vector<std::byte> result(gws * data.sizeOfDataType);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListReset(copyCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(copyCmdList, result.data(), buffer, result.size(), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(copyCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &copyCmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
    for (size_t i = 0; i < gws; i++) {
        if (std::memcmp(result.data() + i * data.sizeOfDataType, expected.expectedValue, data.sizeOfDataType) != 0) {
            testResult = TestResult::VerificationFail;
            break;
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(copyCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(profilingEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    if (arguments.mixGrfModes) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(largeGrfCmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(largeGrfKernel));
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(largeGrfModule));
    }

    return testResult;
}

static RegisterTestCaseImplementation<DoMathOperation> registerTestCase(run, Api::L0);
