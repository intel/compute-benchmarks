/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_shared_migrate_gpu.h"

#include <gtest/gtest.h>

static TestResult run(const UsmSharedMigrateGpuArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::GigabytesPerSecond, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    const DeviceSelection queuePlacement = DeviceSelectionHelper::withoutHost(arguments.bufferPlacement);
    ContextProperties contextProperties = ContextProperties::create().create().setDeviceSelection(arguments.contextPlacement).allowCreationFail();
    QueueProperties queueProperties = QueueProperties::create().setDeviceSelection(queuePlacement).allowCreationFail();
    LevelZero levelzero(queueProperties, contextProperties);
    if (levelzero.context == nullptr || levelzero.commandQueue == nullptr) {
        return TestResult::DeviceNotCapable;
    }
    Timer timer;

    void *bufferVoid{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.bufferPlacement, levelzero, arguments.bufferSize, &bufferVoid));
    int32_t *buffer = static_cast<int32_t *>(bufferVoid);
    const size_t elementsCount = arguments.bufferSize / sizeof(uint32_t);
    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    if (auto result = L0::KernelHelper::loadKernel(levelzero, levelzero.getDevice(queuePlacement), "multitile_memory_benchmark_fill_with_ones.cl", "fill_with_ones", &kernel, &module, nullptr, ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY);
        result != TestResult::Success) {
        return result;
    }

    const uint32_t wgs = 256;
    const uint32_t wgc = static_cast<uint32_t>(elementsCount) / wgs;
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, wgs, 1, 1));
    const ze_group_count_t dispatchTraits{wgc, 1, 1};
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.getDevice(queuePlacement), &cmdListDesc, &cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &dispatchTraits, nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));

    for (auto i = 0u; i < arguments.iterations; i++) {
        for (auto elementIndex = 0u; elementIndex < elementsCount; elementIndex++) {
            buffer[elementIndex] = 0;
        }
        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, 1, &cmdList, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        timer.measureEnd();

        statistics.pushValue(timer.get(), arguments.bufferSize, typeSelector.getUnit(), typeSelector.getType());
    }

    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffer));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmSharedMigrateGpu> registerTestCase(run, Api::L0);
