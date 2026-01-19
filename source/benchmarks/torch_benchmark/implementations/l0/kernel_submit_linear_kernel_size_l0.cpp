/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/l0/levelzero.h"
#include "framework/utility/combo_profiler.h"
#include "definitions/kernel_submit_linear_kernel_size.h"
#include "kernel_submit_common.hpp"

using data_type = double;
const int WARMUP_ITERATIONS = 3;

// static TestResult run_kernel(data_type *d_out, ze_command_list_handle_t cmdList, ze_kernel_handle_t kernel) {
//     ze_group_count_t dispatch{static_cast<uint32_t>(1), 1, 1};
//     ze_group_size_t groupSizes = {static_cast<uint32_t>(1), 1, 1};

//     void *kernelArguments[1] = {&d_out};

//     ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));
//     ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, dispatch, groupSizes,
//                                                                           kernelArguments, nullptr, nullptr, 0, nullptr));

//     return TestResult::Success;
// }

static TestResult run(const KernelSubmitLinearKernelSizeArguments &arguments, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // Setup
    // do in order queue
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(
        arguments.inOrderQueue);
    // inicjalizuje driver L0 i poeira GPU device
    LevelZero levelzero(extensionProperties);

    if (arguments.kernelSize != 32 &&
        arguments.kernelSize != 128 &&
        arguments.kernelSize != 512 &&
        arguments.kernelSize != 1024 &&
        arguments.kernelSize != 5120) {
        std::cerr << "Invalid kernel size: " << arguments.kernelSize << ". Allowed sizes are 32, 128, 512, 1024, 5120." << std::endl;
        return TestResult::Error;
    }

    // Create immediate command list
    // wysyła listę komend do GPU do wykonania
    ze_command_queue_desc_t commandQueueDesc{ZE_STRUCTURE_TYPE_COMMAND_QUEUE_DESC};
    commandQueueDesc.mode = ZE_COMMAND_QUEUE_MODE_ASYNCHRONOUS;
    // z tą flagą komendy są wykonywane w kolejności
    if (arguments.inOrderQueue) {
        commandQueueDesc.flags = ZE_COMMAND_QUEUE_FLAG_IN_ORDER;
    }
    // uchwyt do listy komend
    ze_command_list_handle_t cmdList;
    // to tworzy listę komend, która będzie wykonywana natychmiast po dodaniu komend
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListCreateImmediate(levelzero.context, levelzero.device, &commandQueueDesc, &cmdList));

    // allocate device memory
    data_type *d_out = nullptr;
    l0_malloc_device<data_type>(levelzero, 1, d_out);
    if (!d_out) {
        zeCommandListDestroy(cmdList);
        return TestResult::Error;
    }

    // create kernel
    // ze_kernel_handle_t kernel{};
    // ze_module_handle_t module{};
    const auto kernelName = "torch_benchmark_linear_kernel_size_" + std::to_string(arguments.kernelSize);
    // to zamiast tego
    // ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, kernelName + ".cl", kernelName, kernel, module));
    Kernel kernel(levelzero, kernelName + ".cl", kernelName);
    
    auto run_kernel = [&]() -> TestResult {
        ze_group_count_t dispatch{static_cast<uint32_t>(1), 1, 1};
        ze_group_size_t groupSizes = {static_cast<uint32_t>(1), 1, 1};

        void *kernelArguments[1] = {&d_out};

        // zamiast tego 
        // ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel, 1, 1, 1));
        // ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel, dispatch, groupSizes,
        //                                                                 kernelArguments, nullptr, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeKernelSetGroupSize(kernel.get(), 1, 1, 1));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel.get(), dispatch, groupSizes,
                                                                            kernelArguments, nullptr, nullptr, 0, nullptr));

        return TestResult::Success;
    };
    // warmup
    for (int i = 0; i < WARMUP_ITERATIONS; i++) {
        ASSERT_TEST_RESULT_SUCCESS(run_kernel());
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    // benchmarking
    // for (size_t i = 0; i < args.iterations; ++i) {
    //     profiler.measureStart();
    //     ASSERT_TEST_RESULT_SUCCESS(run_kernel(d_out, l0_ctx, kernel));
    //     profiler.measureEnd();
    //     profiler.pushStats(statistics);

    //     // expect a wait here after a batch of submissions, if batch > 0
    //     if (args.kernelBatchSize > 0 && ((i + 1) % args.kernelBatchSize) == 0) {
    //         ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));
    //     }
    // }
    // ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));
    // benchmarking 
    {
        BatchingLoop batchLoop(cmdList, arguments.kernelBatchSize);
        for (size_t i = 0; i < arguments.iterations; ++i) {
            profiler.measureStart();
            ASSERT_TEST_RESULT_SUCCESS(run_kernel());
            profiler.measureEnd();
            profiler.pushStats(statistics);
            batchLoop.checkBatch(i);
        }
    } 
    
    // verify and clean up
    data_type host_result[1] = {0};
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmdList, &host_result, d_out, sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdList, UINT64_MAX));

    if (host_result[0] > (static_cast<data_type>(arguments.kernelSize) + 0.1) || host_result[0] < (static_cast<data_type>(arguments.kernelSize) - 0.1)) {
        std::cout << "Wrong checker value: " << host_result[0] << ", expected " << static_cast<data_type>(arguments.kernelSize) << std::endl;
        zeCommandListDestroy(cmdList);
        zeMemFree(levelzero.context, d_out);
        return TestResult::Error;
    }

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListDestroy(cmdList));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(levelzero.context, d_out));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitLinearKernelSize> registerTestCase(run, Api::L0, true);
