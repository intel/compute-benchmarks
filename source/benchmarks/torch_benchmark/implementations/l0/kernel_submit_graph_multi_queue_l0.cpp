/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_graph_multi_queue.h"

using data_type = float;

template <typename T>
TestResult verify_result(ze_command_list_handle_t cmd_list, size_t length, data_type *d_a, data_type *d_result) {
    TestResult result = TestResult::Success;

    std::vector<data_type> h_a(length);
    std::vector<data_type> h_b(length);
    std::vector<data_type> h_c(length);
    std::vector<data_type> h_result(length);

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmd_list, h_a.data(), d_a, length * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(cmd_list, h_result.data(), d_result, length * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list, UINT64_MAX));

    // compute expected result
    std::vector<data_type> expected_result(length);
    for (size_t i = 0; i < length; ++i) {
        h_b[i] = h_a[i] + 1.0f;               // first kernel: kernel_add_const
        h_c[i] = h_b[i] + 1.0f;               // second kernel: kernel_add_const
        expected_result[i] = h_b[i] + h_c[i]; // third kernel: kernel_add_arrays
    }

    // verify
    for (size_t i = 0; i < length; ++i) {
        if (std::abs(h_result[i] - expected_result[i]) > static_cast<data_type>(epsilon)) {
            std::cerr << "Verification failed at index " << i << ": expected " << expected_result[i]
                      << ", got " << h_result[i] << std::endl;
            result = TestResult::VerificationFail;
        }
    }

    return result;
}

static TestResult run(const KernelSubmitGraphMultiQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // setup
    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setGraphFunctions(true);
    LevelZero l0{extensionProperties};
    CommandList cmd_list_1{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};
    CommandList cmd_list_2{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};
    CommandList cmd_list_3{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    const size_t length = wgc * wgs;
    DeviceMemory<data_type> d_a{l0, length};
    DeviceMemory<data_type> d_b{l0, length};
    DeviceMemory<data_type> d_c{l0, length};
    DeviceMemory<data_type> d_d{l0, length};

    // initialize d_a for the verification phase
    const data_type init_value = 1.0f;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(cmd_list_1.get(), d_a.getPtr(), &init_value, sizeof(data_type), length * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list_1.get(), UINT64_MAX));

    // create kernels
    Kernel kernel_add_const{l0, "torch_benchmark_add_element_constant.cl", "add_element_constant"};
    Kernel kernel_add_arrays{l0, "torch_benchmark_elementwise_sum_2.cl", "elementwise_sum_2_float"};

    // create events for synchronization between queues
    CounterBasedEvent event_1{l0, args.useProfiling};
    CounterBasedEvent event_2{l0, args.useProfiling};

    // submit kernels
    ze_group_count_t dispatch{wgc, 1, 1};
    ze_group_size_t groupSizes{wgs, 1, 1};
    data_type add_element = 1.0f;
    void *args_1[] = {d_b.getAddress(), d_a.getAddress(), &add_element};
    void *args_2[] = {d_c.getAddress(), d_b.getAddress(), &add_element};
    void *args_3[] = {d_d.getAddress(), d_b.getAddress(), d_c.getAddress()};
    auto submit_kernels = [&]() {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_add_const.get(), dispatch, groupSizes,
                                                                              args_1, nullptr, event_1.get(), 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_2.get(), kernel_add_const.get(), dispatch, groupSizes,
                                                                              args_2, nullptr, event_2.get(), 1, event_1.getAddress()));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_add_arrays.get(), dispatch, groupSizes,
                                                                              args_3, nullptr, nullptr, 1, event_2.getAddress()));
        return TestResult::Success;
    };

    // capture graph
    Graph graph{l0};
    ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListBeginCaptureIntoGraph(cmd_list_1.get(), graph.get(), nullptr));
    for (size_t i = 0; i < args.kernelsPerQueue; ++i) {
        ASSERT_TEST_RESULT_SUCCESS(submit_kernels());
    }
    ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListEndGraphCapture(cmd_list_1.get(), graph.getAddress(), nullptr));

    // instantiate graph
    ASSERT_TEST_RESULT_SUCCESS(graph.instantiate());

    // benchmark: run captured graph
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        // Currently there is an implicit sync inside execute_graph call in SYCL API
        // so only measuring exec + sync is meaningful in terms of comparing
        // perf with SYCL
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmd_list_3.get(), graph.getExecutable(), nullptr, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list_3.get(), UINT64_MAX));

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list_3.get(), UINT64_MAX));

    // verify result
    ASSERT_TEST_RESULT_SUCCESS(verify_result<data_type>(cmd_list_3.get(), length, d_a.getPtr(), d_d.getPtr()));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphMultiQueue> registerTestCase(run, Api::L0, true);
