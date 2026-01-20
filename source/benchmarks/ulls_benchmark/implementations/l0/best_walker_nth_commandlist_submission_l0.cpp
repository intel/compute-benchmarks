/*
 * Copyright (C) 2023-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/file_helper.h"
#include "framework/utility/timer.h"

#include "definitions/best_walker_nth_commandlist_submission.h"

#include <emmintrin.h>
#include <gtest/gtest.h>

struct CmdListSet {
    void *buffer = nullptr;
    ze_command_list_handle_t cmdList = nullptr;
    ze_module_handle_t module = nullptr;
    ze_kernel_handle_t kernel = nullptr;
};

static TestResult run(const BestWalkerNthCommandListSubmissionArguments &arguments, Statistics &statistics) {
    MeasurementFields typeSelector(MeasurementUnit::Microseconds, MeasurementType::Cpu);

    if (isNoopRun()) {
        statistics.pushUnitAndType(typeSelector.getUnit(), typeSelector.getType());
        return TestResult::Nooped;
    }
    if (arguments.cmdListCount == 0) {
        return TestResult::InvalidArgs;
    }

    LevelZero levelzero;
    constexpr static auto bufferSize = 4096u;
    Timer timer;

    std::vector<CmdListSet> cmdListData(arguments.cmdListCount);
    std::vector<ze_command_list_handle_t> cmdLists(arguments.cmdListCount);

    const ze_host_mem_alloc_desc_t allocationDesc{ZE_STRUCTURE_TYPE_HOST_MEM_ALLOC_DESC, nullptr, ZE_HOST_MEM_ALLOC_FLAG_BIAS_UNCACHED};

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
    kernelDesc.pKernelName = "write_one_uncached";

    const ze_group_count_t groupCount{1, 1, 1};
    ze_command_list_desc_t cmdListDesc{};
    cmdListDesc.commandQueueGroupOrdinal = levelzero.commandQueueDesc.ordinal;

    void *buffer = nullptr;
    volatile uint64_t *volatileBuffer = nullptr;

    for (uint32_t i = 0; i < arguments.cmdListCount; i++) {
        ze_command_list_handle_t cmdList{};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreate(levelzero.context, levelzero.device, &cmdListDesc, &cmdList));
        cmdListData[i].cmdList = cmdList;
        cmdLists[i] = cmdList;

        ASSERT_ZE_RESULT_SUCCESS(zeMemAllocHost(levelzero.context, &allocationDesc, bufferSize, 0, &buffer));
        cmdListData[i].buffer = buffer;
        volatileBuffer = static_cast<uint64_t *>(buffer);

        ze_module_handle_t module{};
        ASSERT_ZE_RESULT_SUCCESS(zeModuleCreate(levelzero.context, levelzero.device, &moduleDesc, &module, nullptr));
        cmdListData[i].module = module;

        ze_kernel_handle_t kernel{};
        ASSERT_ZE_RESULT_SUCCESS(zeKernelCreate(module, &kernelDesc, &kernel));
        cmdListData[i].kernel = kernel;

        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetArgumentValue(kernel, 0, sizeof(buffer), &buffer));

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernel(cmdList, kernel, &groupCount, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListClose(cmdList));
    }

    // Benchmark
    for (auto i = 0u; i < arguments.iterations; i++) {
        *volatileBuffer = 0;
        _mm_clflush(buffer);

        timer.measureStart();
        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueExecuteCommandLists(levelzero.commandQueue, static_cast<uint32_t>(arguments.cmdListCount), cmdLists.data(), nullptr));
        while (*volatileBuffer != 1) {
        }
        timer.measureEnd();

        ASSERT_ZE_RESULT_SUCCESS(zeCommandQueueSynchronize(levelzero.commandQueue, std::numeric_limits<uint64_t>::max()));
        statistics.pushValue(timer.get(), typeSelector.getUnit(), typeSelector.getType());
    }

    for (uint32_t i = 0; i < arguments.cmdListCount; i++) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(cmdListData[i].kernel));
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(cmdListData[i].module));
        ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, cmdListData[i].buffer));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdListData[i].cmdList));
    }

    return TestResult::Success;
}

static RegisterTestCaseImplementation<BestWalkerNthCommandListSubmission> registerTestCase(run, Api::L0);
