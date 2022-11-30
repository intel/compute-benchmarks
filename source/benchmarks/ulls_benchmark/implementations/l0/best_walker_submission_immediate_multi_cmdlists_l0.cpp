/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/best_walker_submission_immediate_multi_cmdlists.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

static TestResult run(const BestWalkerSubmissionImmediateMultiCmdlistsArguments &arguments, Statistics &statistics) {
    LevelZero levelzero{QueueProperties::create().disable()};
    constexpr static auto bufferSize = 4096u;
    Timer timer;

    // Create events
    ze_event_pool_desc_t eventPoolDesc{ZE_STRUCTURE_TYPE_EVENT_POOL_DESC};
    eventPoolDesc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;
    eventPoolDesc.count = static_cast<uint32_t>(arguments.cmdlistCount);
    ze_event_pool_handle_t eventPool;
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolCreate(levelzero.context, &eventPoolDesc, 1, &levelzero.device, &eventPool));
    ze_event_desc_t eventDesc{ZE_STRUCTURE_TYPE_EVENT_DESC};
    eventDesc.signal = ZE_EVENT_SCOPE_FLAG_DEVICE;
    eventDesc.wait = ZE_EVENT_SCOPE_FLAG_HOST;
    std::vector<ze_event_handle_t> events(arguments.cmdlistCount);
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        eventDesc.index = i;
        ASSERT_ZE_RESULT_SUCCESS(zeEventCreate(eventPool, &eventDesc, &events[i]));
    }

    // Create buffers
    ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC};
    std::vector<void *> buffers(arguments.cmdlistCount);
    std::vector<volatile uint64_t *> volatileBuffers(arguments.cmdlistCount);
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffers[i]));
        volatileBuffers[i] = static_cast<uint64_t *>(buffers[i]);
    }

    // Create and configure kernels
    const auto kernelBinary = FileHelper::loadBinaryFile("ulls_benchmark_write_one.spv");
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
    kernelDesc.pKernelName = "write_one";
    std::vector<ze_kernel_handle_t> kernels(arguments.cmdlistCount);
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernels[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernels[i], 1, 1, 1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernels[i], 0, sizeof(buffers[i]), &buffers[i]));
    }

    // Create an immediate command lists
    const ze_group_count_t groupCount{1, 1, 1};
    std::vector<ze_command_list_handle_t> cmdLists(arguments.cmdlistCount);
    auto commandQueueDesc = QueueFamiliesHelper::getPropertiesForSelectingEngine(levelzero.device, Engine::Ccs0);
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc->desc, &cmdLists[i]));
    }

    // Warmup
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, events[i], 0, nullptr));
    }
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[i], std::numeric_limits<uint64_t>::max()));
        ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
    }

    // Benchmark
    for (auto j = 0u; j < arguments.iterations; j++) {
        for (auto i = 0u; i < arguments.cmdlistCount; i++) {
            *volatileBuffers[i] = 0;
            _mm_clflush(buffers[i]);
        }

        timer.measureStart();
        for (auto i = 0u; i < arguments.cmdlistCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdLists[i], kernels[i], &groupCount, events[i], 0, nullptr));
        }
        for (auto i = 0u; i < arguments.cmdlistCount; i++) {
            while (*volatileBuffers[i] != 1) {
            }
        }
        timer.measureEnd();

        for (auto i = 0u; i < arguments.cmdlistCount; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostSynchronize(events[i], std::numeric_limits<uint64_t>::max()));
            ASSERT_ZE_RESULT_SUCCESS(zeEventHostReset(events[i]));
        }
        statistics.pushValue(timer.get(), MeasurementUnit::Microseconds, MeasurementType::Cpu);
    }

    // Cleanup
    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernels[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    for (auto i = 0u; i < arguments.cmdlistCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, buffers[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdLists[i]));
        ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(events[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeEventPoolDestroy(eventPool));
    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerSubmissionImmediateMultiCmdlists> registerTestCase(run, Api::L0);
