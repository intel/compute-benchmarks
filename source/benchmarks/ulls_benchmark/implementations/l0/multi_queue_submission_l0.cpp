/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/multi_queue_submission.h"

#include <gtest/gtest.h>

static TestResult run(const MultiQueueSubmissionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    QueueProperties queueProperties = QueueProperties::create().disable();
    LevelZero levelzero(queueProperties);
    Timer timer;

    if (arguments.workgroupSize > levelzero.getDeviceComputeProperties().maxTotalGroupSize) {
        return TestResult::DeviceNotCapable;
    }

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("ulls_benchmark_write_one_global_ids.spv");
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
    kernelDesc.pKernelName = "write_one";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    uint32_t groupSizeX = static_cast<uint32_t>(arguments.workgroupSize);
    uint32_t groupSizeY = 1u;
    uint32_t groupSizeZ = 1u;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, groupSizeX, groupSizeY, groupSizeZ));

    ze_group_count_t dispatchTraits;
    dispatchTraits.groupCountX = static_cast<uint32_t>(arguments.workgroupCount);
    dispatchTraits.groupCountY = 1u;
    dispatchTraits.groupCountZ = 1u;

    // Check how many compute queues we can run
    auto queueFamilies = QueueFamiliesHelper::queryQueueFamilies(levelzero.device);
    size_t computeQueuesCount = 0;
    for (const auto &queueFamily : queueFamilies) {
        if (queueFamily.type == EngineGroup::Compute) {
            computeQueuesCount = queueFamily.queueCount;
            break;
        }
    }
    if (0 == computeQueuesCount) {
        return TestResult::DeviceNotCapable;
    }

    // Create command queues, command lists and buffers
    size_t gws = arguments.workgroupCount * arguments.workgroupSize;
    size_t size = gws * sizeof(int);
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    std::vector<ze_command_queue_handle_t> queues(arguments.queueCount);
    std::vector<ze_command_list_handle_t> cmdLists(arguments.queueCount);
    std::vector<void *> buffers(arguments.queueCount);
    for (size_t i = 0; i < arguments.queueCount; i++) {
        commandQueueDesc.index = static_cast<uint32_t>(i % computeQueuesCount);
        queues[i] = levelzero.createQueue(levelzero.device, commandQueueDesc);

        ze_command_list_desc_t cmdListDesc{};
        cmdListDesc.commandQueueGroupOrdinal = commandQueueDesc.ordinal;
        ze_command_list_handle_t cmdList;
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
        cmdLists[i] = cmdList;

        const ze_device_mem_alloc_desc_t deviceAllocDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
        void *pBuffer = nullptr;
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocDesc, size, 0, levelzero.device, &pBuffer));
        buffers[i] = pBuffer;

        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(void *), &pBuffer));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    }

    // Warmup
    for (size_t i = 0; i < arguments.queueCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queues[i], 1, &cmdLists[i], nullptr));
    }
    for (size_t i = 0; i < arguments.queueCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queues[i], std::numeric_limits<uint64_t>::max()));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        timer.measureStart();
        for (size_t j = 0; j < arguments.queueCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(queues[j], 1, &cmdLists[j], nullptr));
        }
        for (size_t j = 0; j < arguments.queueCount; j++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(queues[j], std::numeric_limits<uint64_t>::max()));
        }
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    for (size_t i = 0; i < arguments.queueCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdLists[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffers[i]));
    }
    return TestResult::Success;
}

static RegisterTestCaseImplementation<MultiQueueSubmission> registerTestCase(run, Api::L0);
