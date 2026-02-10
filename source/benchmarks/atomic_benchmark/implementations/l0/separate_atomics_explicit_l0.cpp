/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/math_operation_helper.h"
#include "framework/utility/timer.h"

#include "definitions/separate_atomic_explicit.h"
#include "kernel_helper.h"

#include <cstring>
#include <gtest/gtest.h>
#include <level_zero/zer_api.h>

static TestResult run(const SeparateAtomicsExplicitArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Nanoseconds, arguments.useEvents ? MeasurementType::Gpu : MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(true);
    LevelZero levelzero(extensionProperties);
    Timer timer{};

    // Check support
    if (!MathOperationHelper::isSupportedAsAtomic(arguments.atomicOperation, arguments.dataType, levelzero.isGlobalFloatAtomicsSupported(), false)) {
        return TestResult::DeviceNotCapable;
    }

    // Prepare data
    const size_t lws = arguments.workgroupSize;
    const size_t gws = arguments.workgroupSize * arguments.workgroupCount;
    const size_t threadsPerAtomicCount = arguments.iterations;
    const auto data = KernelHelper::getDataForKernel(arguments.dataType, arguments.atomicOperation, threadsPerAtomicCount);

    // Buffer sizes
    const size_t cachelinesCount = gws / arguments.atomicsPerCacheline;
    const size_t atomicBufferSize = cachelinesCount * MemoryConstants::cachelineSize;
    const size_t otherArgumentsBufferEntryCount = 4u; // we only need 1 value, but storing in multiple can prevent some compiler opts
    const size_t otherArgumentsBufferSize = otherArgumentsBufferEntryCount * data.sizeOfDataType;

    // Prepare timestamp event
    ze_event_handle_t perfEvent{};
    auto eventDesc = defaultCounterBasedEventDesc;
    eventDesc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;
    ASSERT_ZE_RESULT_SUCCESS(levelzero.counterBasedEventCreate2(levelzero.context, levelzero.device, &eventDesc, &perfEvent));

    // Create kernel
    const char *programName = "atomic_benchmark_kernel.cl";
    auto sourceBytes = FileHelper::loadBinaryFile(programName);
    if (sourceBytes.size() == 0) {
        return TestResult::KernelNotFound;
    }
    sourceBytes.push_back('\0');
    const std::string compilerOptions = KernelHelper::getCompilerOptionsExplicit(arguments.dataType, arguments.atomicOperation, arguments.memoryOrder, arguments.scope, otherArgumentsBufferEntryCount);
    const std::string extraArgs{"-spv_only"};
    ze_module_handle_t module{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_OCLC;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(sourceBytes.data());
    moduleDesc.inputSize = sourceBytes.size();
    moduleDesc.pBuildFlags = compilerOptions.c_str();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));

    ze_kernel_handle_t initializeKernel{};
    ze_kernel_desc_t kernelDesc1{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc1.pKernelName = "initialize";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc1, &initializeKernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(initializeKernel, 1u, 1u, 1u));

    ze_kernel_handle_t kernel{};
    ze_kernel_desc_t kernelDesc2{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc2.pKernelName = "separate_atomics";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc2, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(lws), 1u, 1u));

    // Create and initialize the atomic buffer with kernel (we need to use atomic_init for explicit atomics)
    ze_command_list_handle_t immCmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &immCmdList));

    void *atomicBuffer = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, atomicBufferSize, 0, levelzero.device, &atomicBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(initializeKernel, 0, sizeof(atomicBuffer), &atomicBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(initializeKernel, 1, data.sizeOfDataType, &data.initialValue));
    const uint32_t gwsForInitialize = static_cast<uint32_t>(cachelinesCount * (MemoryConstants::cachelineSize / data.sizeOfDataType));
    const ze_group_count_t initGroupCount{gwsForInitialize, 1u, 1u};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(immCmdList, initializeKernel, &initGroupCount, nullptr, 0, nullptr));

    // Create and initialize the buffer with value for the other argument of atomic operation
    void *otherArgumentsBuffer = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &zeDefaultGPUDeviceMemAllocDesc, otherArgumentsBufferSize, 0, levelzero.device, &otherArgumentsBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(immCmdList, otherArgumentsBuffer, data.otherArgument, data.sizeOfDataType, otherArgumentsBufferSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(immCmdList, std::numeric_limits<uint64_t>::max()));
    uint32_t iterations = static_cast<uint32_t>(data.loopIterations);

    // Benchmark
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(atomicBuffer), &atomicBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 1, sizeof(otherArgumentsBuffer), &otherArgumentsBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 2, sizeof(iterations), &iterations));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 3, arguments.atomicsPerCacheline.getSizeOf(), arguments.atomicsPerCacheline.getAddressOf()));
    const ze_group_count_t groupCount{static_cast<uint32_t>(gws / lws), 1u, 1u};

    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(immCmdList, kernel, &groupCount, arguments.useEvents ? perfEvent : nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(immCmdList, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        auto totalAtomicOperations = data.loopIterations * data.operatorApplicationsPerIteration;
        if (arguments.useEvents) {
            ze_kernel_timestamp_result_t kernelTimestamp{};
            ASSERT_ZE_RESULT_SUCCESS(zeEventQueryKernelTimestamp(perfEvent, &kernelTimestamp));
            auto commandTime = levelzero.getAbsoluteKernelExecutionTime(kernelTimestamp.global);
            auto timePerAtomicOperation = commandTime / totalAtomicOperations;
            statistics.pushValue(std::chrono::nanoseconds(timePerAtomicOperation), typeSelector.getUnit(), typeSelector.getType());
        } else {
            auto timePerAtomicOperation = timer.get() / totalAtomicOperations;
            statistics.pushValue(timePerAtomicOperation, typeSelector.getUnit(), typeSelector.getType());
        }
    }

    // Verify
    auto result = std::make_unique<std::byte[]>(atomicBufferSize);
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(immCmdList, result.get(), atomicBuffer, atomicBufferSize, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(immCmdList, std::numeric_limits<uint64_t>::max()));
    for (auto cachelineIndex = 0u; cachelineIndex < cachelinesCount; cachelineIndex++) {
        for (auto atomicIndex = 0u; atomicIndex < MemoryConstants::cachelineSize / data.sizeOfDataType; atomicIndex++) {
            const bool wasTouched = atomicIndex < arguments.atomicsPerCacheline;
            const std::byte *expectedValue = wasTouched ? data.expectedValue : data.initialValue;
            const std::byte *actualValue = result.get() + cachelineIndex * MemoryConstants::cachelineSize + atomicIndex * data.sizeOfDataType;
            if (std::memcmp(actualValue, expectedValue, data.sizeOfDataType) != 0) {
                return TestResult::VerificationFail;
            }
        }
    }

    // Cleanup
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(perfEvent));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(immCmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(initializeKernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, atomicBuffer));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, otherArgumentsBuffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<SeparateAtomicsExplicit> registerTestCase(run, Api::L0);
