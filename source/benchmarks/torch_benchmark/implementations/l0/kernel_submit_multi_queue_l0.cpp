/*
 * Copyright (C) 2024-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_multi_queue.h"

using data_type = int;

constexpr int DATA_NUM = 2;

static TestResult run(const KernelSubmitMultiQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    ExtensionProperties extensionProperties = ExtensionProperties::create().setCounterBasedCreateFunctions(true);
    LevelZero l0{extensionProperties};
    CommandList cmd_list_1{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};
    CommandList cmd_list_2{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};
    const size_t length = args.kernelWGCount * args.kernelWGSize;

    std::vector<DeviceMemory<data_type>> d_a{};
    std::vector<DeviceMemory<data_type>> d_b{};
    std::vector<DeviceMemory<data_type>> d_c{};
    for (int i = 0; i < DATA_NUM; i++) {
        d_a.emplace_back(DeviceMemory<data_type>(l0, length));
        d_b.emplace_back(DeviceMemory<data_type>(l0, length));
        d_c.emplace_back(DeviceMemory<data_type>(l0, length));
    }

    // create kernel
    Kernel kernel{l0, "torch_benchmark_elementwise_sum_2.cl", "elementwise_sum_2_int"};
    const ze_group_count_t dispatch{static_cast<uint32_t>(args.kernelWGCount), 1u, 1u};
    const ze_group_size_t groupSizes{static_cast<uint32_t>(args.kernelWGSize), 1u, 1u};
    void *kernel_args_0[3] = {d_c[0].getAddress(), d_a[0].getAddress(), d_b[0].getAddress()};
    void *kernel_args_1[3] = {d_c[1].getAddress(), d_a[1].getAddress(), d_b[1].getAddress()};

    auto submit_kernel = [&](ze_command_list_handle_t cmd_list,
                             void **kernel_args,
                             ze_event_handle_t signal_event,
                             ze_event_handle_t wait_event) -> TestResult {
        for (int i = 0; i < DATA_NUM; i++) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list, kernel.get(), dispatch, groupSizes,
                                                                                  kernel_args, nullptr, signal_event,
                                                                                  wait_event ? 1 : 0,
                                                                                  wait_event ? &wait_event : nullptr));
        }
        return TestResult::Success;
    };

    // create counter-based event to sync cmd_list_1 and cmd_list_2
    CounterBasedEvent q2_last_event{l0, args.useProfiling};

    // benchmark
    for (size_t i = 0; i < args.iterations; i++) {
        if (args.measureCompletionTime)
            profiler.measureStart();

        // submit several kernels into cmd_list_1
        for (size_t j = 0; j < args.kernelsPerQueue; j++) {
            ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmd_list_1.get(), kernel_args_0, nullptr, nullptr));
        }
        // submit several kernels into cmd_list_2
        for (size_t j = 1; j < args.kernelsPerQueue; j++) {
            ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmd_list_2.get(), kernel_args_1, nullptr, nullptr));
        }
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmd_list_2.get(), kernel_args_1, q2_last_event.get(), nullptr));
        // mark the last kernel in cmd_list_2
        if (!args.measureCompletionTime)
            profiler.measureStart();
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmd_list_1.get(), kernel_args_0, nullptr, q2_last_event.get()));
        if (!args.measureCompletionTime) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }

        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list_1.get(), UINT64_MAX));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list_2.get(), UINT64_MAX));

        if (args.measureCompletionTime) {
            profiler.measureEnd();
            profiler.pushStats(statistics);
        }
    }

    return TestResult::Success;
};

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitMultiQueue> registerTestCase(run, Api::L0, true);
