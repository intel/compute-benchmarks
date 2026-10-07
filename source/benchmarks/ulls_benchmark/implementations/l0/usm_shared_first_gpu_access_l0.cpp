/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_first_gpu_access.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedFirstGpuAccessArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    LevelZero levelzero;
    Timer timer;

    ze_host_mem_alloc_desc_t hostAllocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    if (arguments.initialPlacement == UsmInitialPlacement::Host) {
        hostAllocationDesc.flags = ZE_HOST_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT;
    }
    ze_device_mem_alloc_desc_t deviceAllocationDesc{ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC};
    if (arguments.initialPlacement == UsmInitialPlacement::Device) {
        deviceAllocationDesc.flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_INITIAL_PLACEMENT;
    }
    void *buffer{};

    ze_module_handle_t module;
    ze_kernel_handle_t kernel;
    if (auto result = L0::KernelHelper::loadKernel(levelzero, "ulls_benchmark_write_one.cl", "write_one", &kernel, &module, nullptr, ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY);
        result != TestResult::Success) {
        return result;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));

    const ze_group_count_t groupCount{1, 1, 1};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList{};

    // Keep the previous allocation alive while creating the next one so the driver hands out a fresh VA
    void *previousBuffer = nullptr;

    for (auto i = 0u; i < arguments.iterations; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocShared(levelzero.context, &deviceAllocationDesc, &hostAllocationDesc, arguments.bufferSize, 0, levelzero.device, &buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
        if (previousBuffer != nullptr) {
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, previousBuffer));
        }
        previousBuffer = buffer;
    }

    if (previousBuffer != nullptr) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, previousBuffer));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedFirstGpuAccess> registerTestCase(run, Api::L0);
