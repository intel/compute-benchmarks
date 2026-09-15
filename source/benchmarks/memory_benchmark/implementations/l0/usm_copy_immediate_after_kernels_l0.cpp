/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/l0/utility/buffer_contents_helper_l0.h"
#include "framework/l0/utility/kernel_helper_l0.h"
#include "framework/l0/utility/usm_helper.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/timer.h"

#include "definitions/usm_copy_immediate_after_kernels.h"

#include <gtest/gtest.h>
#include <level_zero/zer_api.h>

static TestResult run(const UsmCopyImmediateAfterKernelsArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (!isUsmMemoryType(arguments.sourcePlacement) || !isUsmMemoryType(arguments.destinationPlacement)) {
        return TestResult::ApiNotCapable;
    }

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }

    QueueProperties queueProperties = QueueProperties::create();
    ContextProperties contextProperties = ContextProperties::create();
    ExtensionProperties extensionProperties = ExtensionProperties::create().setImportHostPointerFunctions(
        requiresImport(arguments.sourcePlacement) || requiresImport(arguments.destinationPlacement));
    LevelZero levelzero(queueProperties, contextProperties, extensionProperties);

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    const TestResult kernelLoadResult = L0::KernelHelper::loadKernel(levelzero, "api_overhead_benchmark_eat_time.cl", "eat_time", &kernel, &module, nullptr);
    if (kernelLoadResult != TestResult::Success) {
        return kernelLoadResult;
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1u, 1u, 1u));
    int kernelOperationsCount = static_cast<int>(arguments.kernelTime * 4);
    ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(int), &kernelOperationsCount));
    const ze_group_count_t groupCount{1, 1, 1};

    ze_command_list_handle_t cmdList{};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &zeDefaultGPUImmediateCommandQueueDesc, &cmdList));

    void *source{}, *destination{};
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.sourcePlacement, levelzero, arguments.size, &source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::allocate(arguments.destinationPlacement, levelzero, arguments.size, &destination));
    ASSERT_ZE_RESULT_SUCCESS(BufferContentsHelperL0::fillBuffer(levelzero, source, arguments.size, BufferContents::Random, true));

    Timer appendTimer, totalTimer;
    for (auto iteration = 0u; iteration < arguments.iterations; iteration++) {
        totalTimer.measureStart();
        for (size_t kernelIndex = 0; kernelIndex < arguments.numKernels; kernelIndex++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        }
        appendTimer.measureStart();
        for (size_t copyIndex = 0; copyIndex < arguments.numCopies; copyIndex++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, destination, source, arguments.size, nullptr, 0, nullptr));
        }
        appendTimer.measureEnd();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, std::numeric_limits<uint64_t>::max()));
        totalTimer.measureEnd();

        statistics.pushValue(appendTimer.get(), typeSelector.getUnit(), typeSelector.getType(), "Append");
        statistics.pushValue(totalTimer.get(), typeSelector.getUnit(), typeSelector.getType(), "Total");
    }

    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.sourcePlacement, levelzero, source));
    ASSERT_ZE_RESULT_SUCCESS(UsmHelper::deallocate(arguments.destinationPlacement, levelzero, destination));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
}

static RegisterTestCaseImplementation<UsmCopyImmediateAfterKernels> registerTestCase(run, Api::L0);
