/*
 * Copyright (C) 2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/best_walker_nth_submission_immediate.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

struct KernelSet {
    void *buffer = nullptr;
    ze_module_handle_t module = nullptr;
    ze_kernel_handle_t kernel = nullptr;
    ze_event_handle_t event = nullptr;
};

static TestResult run(const BestWalkerNthSubmissionImmediateArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    if (arguments.kernelCount == 0) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero{QueueProperties::create().disable()};
    constexpr static auto bufferSize = 4096u;
    Timer timer;

    std::vector<KernelSet> kernels(arguments.kernelCount);

    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = arguments.kernelCount;
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));

    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;

    ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    void *buffer = nullptr;
    volatile uint64_t *volatileBuffer = nullptr;

    const auto kernelBinary = FileHelper::loadBinaryFile("ulls_benchmark_write_one.spv");
    if (kernelBinary.size() == 0) {
        return TestResult::KernelNotFound;
    }
    ze_module_desc_t moduleDesc{ZE_STRUCTURE_TYPE_MODULE_DESC};
    moduleDesc.format = ZE_MODULE_FORMAT_IL_SPIRV;
    moduleDesc.inputSize = kernelBinary.size();
    moduleDesc.pInputModule = kernelBinary.data();

    ze_kernel_desc_t kernelDesc{ZE_STRUCTURE_TYPE_KERNEL_DESC};
    kernelDesc.flags = ZE_KERNEL_FLAG_EXPLICIT_RESIDENCY;
    kernelDesc.pKernelName = "write_one";

    for (uint32_t i = 0; i < arguments.kernelCount; i++) {
        eventDesc.index = i;
        ze_event_handle_t event{};
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &event));
        kernels[i].event = event;

        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
        kernels[i].buffer = buffer;
        volatileBuffer = static_cast<uint64_t *>(buffer);

        ze_module_handle_t module{};
        ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
        kernels[i].module = module;

        ze_kernel_handle_t kernel{};
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
        kernels[i].kernel = kernel;

        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));
    }

    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    ze_command_list_handle_t cmdList;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    const ze_group_count_t groupCount{1, 1, 1};

    // Warmup
    for (uint32_t i = 0; i < arguments.kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[i].kernel, &groupCount, kernels[i].event, 0, nullptr));
    }
    for (uint32_t i = 0; i < arguments.kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(kernels[i].event, std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(kernels[i].event));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *volatileBuffer = 0;
        _mm_clflush(buffer);

        timer.measureStart();
        for (uint32_t i = 0; i < arguments.kernelCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernels[i].kernel, &groupCount, kernels[i].event, 0, nullptr));
        }
        while (*volatileBuffer != 1) {
        }
        timer.measureEnd();

        for (uint32_t i = 0; i < arguments.kernelCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(kernels[i].event, std::numeric_limits<uint64_t>::max()));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(kernels[i].event));
        }

        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (uint32_t i = 0; i < arguments.kernelCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i].kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(kernels[i].module));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, kernels[i].buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(kernels[i].event));
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerNthSubmissionImmediate> registerTestCase(run, Api::L0);
