/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_migrate_cpu.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedMigrateCpuArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    // Create buffers
    const ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *buffer{};
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(levelzero.context, &deviceAllocationDesc, &hostAllocationDesc, arguments.bufferSize, 0, levelzero.device, &buffer));
    int32_t *bufferInt = static_cast<int32_t *>(buffer);
    const size_t elementsCount = arguments.bufferSize / sizeof(uint32_t);

    // Create kernel
    const auto kernelBinary = FileHelper::loadBinaryFile("memory_benchmark_fill_with_ones.spv");
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
    kernelDesc.pKernelName = "fill_with_ones";
    ze_kernel_handle_t kernel{};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));

    // Configure dispath parameters
    const uint32_t wgs = 256;
    const uint32_t wgc = static_cast<uint32_t>(elementsCount / wgs);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, wgs, 1, 1));
    const ze_group_count_t dispatchTraits{wgc, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

    // Create command list
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    if (arguments.preferredLocation == UsmMemAdvisePreferredLocation::System) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemAdvise(cmdList, levelzero.device, buffer, arguments.bufferSize, ZE_MEMORY_ADVICE_SET_SYSTEM_MEMORY_PREFERRED_LOCATION));
    }
    if (arguments.preferredLocation == UsmMemAdvisePreferredLocation::Device) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemAdvise(cmdList, levelzero.device, buffer, arguments.bufferSize, ZE_MEMORY_ADVICE_SET_PREFERRED_LOCATION));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

        timer.measureStart();
        if (arguments.accessAllBytes) {
            for (auto elementIndex = 0u; elementIndex < elementsCount; elementIndex++) {
                bufferInt[elementIndex] = 0;
            }
        } else {
            bufferInt[0] = 0;
        }
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.bufferSize, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedMigrateCpu> registerTestCase(run, Api::L0);
