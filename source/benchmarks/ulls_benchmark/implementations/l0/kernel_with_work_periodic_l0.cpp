/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/power_meter.h"
#include "framework/utility/sleep.h"
#include "framework/utility/timer.h"

#include "definitions/kernel_with_work_periodic.h"

#include <gtest/gtest.h>

static TestResult run(const KernelWithWorkPeriodicArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    // Setup
    LevelZero levelzero;
    Timer timer;
    PowerMeter powerMeter;

    // Create output buffer
    const ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    size_t workgroupCount = 1000;
    size_t workgroupSize = 256;
    const auto bufferSize = sizeof(uint32_t) * workgroupCount * workgroupSize;
    ASSERT_ZE_RESULT_SUCCESS(zeMemAllocDevice(levelzero.context, &deviceAllocationDesc, bufferSize, 0, levelzero.device, &buffer));
    ASSERT_ZE_RESULT_SUCCESS(zeContextMakeMemoryResident(levelzero.context, levelzero.device, buffer, bufferSize))

    // Create kernel
    auto spirvModule = FileHelper::loadBinaryFile("ulls_benchmark_write_one_global_ids.spv");
    if (spirvModule.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.pInputModule = reinterpret_cast<const uint8_t *>(spirvModule.data());
    moduleDesc.inputSize = spirvModule.size();
    ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.pKernelName = "write_one";
    ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, static_cast<uint32_t>(workgroupSize), 1u, 1u));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

    // Create command list and append empty kernel
    const ze_group_count_t groupCount{static_cast<uint32_t>(workgroupCount), 1u, 1u};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    // Warmup
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));

    // Benchmark
    const auto submissionDelay = std::chrono::microseconds(arguments.timeBetweenSubmissions);
    for (auto i = 0u; i < arguments.iterations; ++i) {
        const auto sleepToTimeoutUllsController = std::chrono::microseconds(10000);
        sleep(sleepToTimeoutUllsController);
        Timer::Clock::duration executionTime(0);
        ASSERT_ZE_RESULT_SUCCESS(powerMeter.measureStart());
        for (auto numKernel = 0u; numKernel < arguments.numSubmissions; ++numKernel) {
            sleep(submissionDelay);
            timer.measureStart();
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
            timer.measureEnd();
            executionTime += timer.get();
        }
        ASSERT_ZE_RESULT_SUCCESS(powerMeter.measureEnd());
        statistics.pushValue(executionTime, typeSelector.getUnit(), typeSelector.getType());
        if (powerMeter.isEnabled()) {
            statistics.pushValue(powerMeter.getTime(), typeSelector.getUnit(), typeSelector.getType(), "timeWithSleeps");
            statistics.pushEnergy(powerMeter.getEnergy(), MeasurementUnit::MicroJoules, MeasurementType::Gpu, "energy");
            statistics.pushEnergy(powerMeter.getPower(), MeasurementUnit::Watts, MeasurementType::Gpu, "avgPower");
        }
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeContextEvictMemory(levelzero.context, levelzero.device, buffer, bufferSize));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<KernelWithWorkPeriodic> registerTestCase(run, Api::L0);
