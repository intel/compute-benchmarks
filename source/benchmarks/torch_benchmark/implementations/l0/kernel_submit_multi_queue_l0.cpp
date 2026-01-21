/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/utility/combo_profiler.h"

#include "definitions/kernel_submit_multi_queue.h"
#include "kernel_submit_common.hpp"

using data_type = int;

static TestResult launch_kernel_l0(ze_command_list_handle_t cmdListImmediate,
                                   ze_kernel_handle_t kernel,
                                   ze_group_count_t dispatch,
                                   const uint32_t wgs,
                                   data_type *res,
                                   data_type *data_1,
                                   data_type *data_2,
                                   ze_event_handle_t signal_event,
                                   ze_event_handle_t wait_event) {
    void *kernelArguments[3] = {&res, &data_1, &data_2};
    ze_group_size_t groupSizes = {wgs, 1u, 1u};

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdListImmediate, kernel, dispatch, groupSizes,
                                                                          kernelArguments, nullptr, signal_event,
                                                                          wait_event ? 1 : 0,
                                                                          wait_event ? &wait_event : nullptr));
    return TestResult::Success;
}

static TestResult run(const KernelSubmitMultiQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    L0Context l0Ctx{};
    const size_t length = static_cast<size_t>(args.kernelWGCount * args.kernelWGSize);

    ze_kernel_handle_t kernel{};
    ze_module_handle_t module{};
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0Ctx.l0, "torch_benchmark_elementwise_sum_2.cl", "torch_benchmark_elementwise_sum_2", kernel, module));

    data_type *d_a[2] = {};
    data_type *d_b[2] = {};
    data_type *d_c[2] = {};
    // allocate device memory
    for (int i = 0; i < 2; i++) {
        ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0Ctx.l0, length, d_a[i]));
        ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0Ctx.l0, length, d_b[i]));
        ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0Ctx.l0, length, d_c[i]));
    }

    ze_group_count_t dispatch{static_cast<uint32_t>(args.kernelWGCount), 1, 1};

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0Ctx.cmdListImmediate_1, UINT64_MAX));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0Ctx.cmdListImmediate_2, UINT64_MAX));

    // create counter-based event to sync cmdlist_1 and cmdlist_2
    ze_event_handle_t q2_last_event = nullptr;
    zex_counter_based_event_desc_t desc = {};
    desc.stype = ZEX_STRUCTURE_COUNTER_BASED_EVENT_DESC;
    desc.pNext = nullptr;
    desc.flags = ZEX_COUNTER_BASED_EVENT_FLAG_IMMEDIATE;
    if (args.useProfiling)
        desc.flags |= ZEX_COUNTER_BASED_EVENT_FLAG_KERNEL_TIMESTAMP;

    L0CounterBasedEventCreate2 counterBasedEventCreate2 = nullptr;
    ASSERT_ZE_RESULT_SUCCESS(zeDriverGetExtensionFunctionAddress(
        l0Ctx.l0.driver,
        "zexCounterBasedEventCreate2",
        reinterpret_cast<void **>(&counterBasedEventCreate2)));
    if (!counterBasedEventCreate2) {
        throw std::runtime_error("Driver does not support Counter-Based Event");
    }
    ASSERT_ZE_RESULT_SUCCESS(counterBasedEventCreate2(l0Ctx.l0.context, l0Ctx.l0.device, &desc, &q2_last_event));

    for (size_t i = 0; i < args.iterations; i++) {

        if (args.measureCompletion)
            profiler.measureStart();

        // submit several kernels into cmdlist_1
        for (size_t j = 0; j < args.kernelsPerQueue; j++) {

            ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0Ctx.cmdListImmediate_1, kernel, dispatch, static_cast<uint32_t>(args.kernelWGSize), d_a[0], d_b[0], d_c[0], nullptr, nullptr));
        }
        // submit several kernels into cmdlist_2
        for (size_t j = 1; j < args.kernelsPerQueue; j++) {
            ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0Ctx.cmdListImmediate_2, kernel, dispatch, static_cast<uint32_t>(args.kernelWGSize), d_a[1], d_b[1], d_c[1], nullptr, nullptr));
        }
        ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0Ctx.cmdListImmediate_2, kernel, dispatch, static_cast<uint32_t>(args.kernelWGSize), d_a[1], d_b[1], d_c[1], q2_last_event, nullptr));
        // mark the last kernel in cmdlist_2
        if (!args.measureCompletion)
            profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(launch_kernel_l0(l0Ctx.cmdListImmediate_1, kernel, dispatch, static_cast<uint32_t>(args.kernelWGSize), d_a[0], d_b[0], d_c[0], nullptr, q2_last_event));
        if (!args.measureCompletion) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0Ctx.cmdListImmediate_1, UINT64_MAX));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0Ctx.cmdListImmediate_2, UINT64_MAX));

        if (args.measureCompletion) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
    }

    // clean
    for (int i = 0; i < 2; i++) {
        if (d_a[i])
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0Ctx.l0.context, d_a[i]));
        if (d_b[i])
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0Ctx.l0.context, d_b[i]));
        if (d_c[i])
            ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0Ctx.l0.context, d_c[i]));
    }
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module));

    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMultiQueue> registerTestCase(run, Api::L0, true);
