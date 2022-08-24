/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_migrate_gpu.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedMigrateGpuArguments &arguments, Statistics &statistics) {
    const DeviceSelection queuePlacement = DeviceSelectionHelper::withoutHost(arguments.bufferPlacement);
    ContextProperties contextProperties = ContextProperties::create().create().setDeviceSelection(arguments.contextPlacement).allowCreationFail();
    QueueProperties queueProperties = QueueProperties::create().setDeviceSelection(queuePlacement).allowCreationFail();
    LevelZero levelzero(queueProperties, contextProperties);
    if (levelzero.context == nullptr || levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    // Create buffer
    void *bufferVoid{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.bufferPlacement, levelzero, arguments.bufferSize, &bufferVoid));
    int32_t *buffer = static_cast<int32_t *>(bufferVoid);
    const size_t elementsCount = arguments.bufferSize / sizeof(uint32_t);

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("multitile_memory_benchmark_fill_with_ones.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();
    ze_module_handle_t module{};
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.getDevice(queuePlacement), &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "fill_with_ones";
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure dispatch parameters
    const uint32_t wgs = 256;
    const uint32_t wgc = static_cast<uint32_t>(elementsCount) / wgs;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, wgs, 1, 1));
    const ze_group_count_t dispatchTraits{wgc, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.getDevice(queuePlacement), &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    for (auto elementIndex = 0u; elementIndex < elementsCount; elementIndex++) {
        buffer[elementIndex] = 0;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        for (auto elementIndex = 0u; elementIndex < elementsCount; elementIndex++) {
            buffer[elementIndex] = 0;
        }

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.bufferSize, MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedMigrateGpu> registerTestCase(run, Api::L0);
