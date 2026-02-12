/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_graph_multi_queue.h"
#include "kernel_submit_common.hpp"

using data_type = float;

template <typename T>
TestResult verify_result(L0Context &l0_ctx, size_t length, data_type *d_a, data_type *d_result) {
    TestResult result = TestResult::Success;

    std::vector<data_type> h_a(length);
    std::vector<data_type> h_b(length);
    std::vector<data_type> h_c(length);
    std::vector<data_type> h_result(length);

    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(l0_ctx.cmdListImmediate_3, h_a.data(), d_a, length * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryCopy(l0_ctx.cmdListImmediate_3, h_result.data(), d_result, length * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_3, UINT64_MAX));

    // compute expected result
    std::vector<data_type> expected_result(length);
    for (size_t i = 0; i < length; ++i) {
        h_b[i] = h_a[i] + 1.0f;               // first kernel: kernel_add_const
        h_c[i] = h_b[i] + 1.0f;               // second kernel: kernel_add_const
        expected_result[i] = h_b[i] + h_c[i]; // third kernel: kernel_add_arrays
    }

    // verify
    const data_type epsilon = static_cast<data_type>(1e-5);
    for (size_t i = 0; i < length; ++i) {
        if (std::abs(h_result[i] - expected_result[i]) > epsilon) {
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
                                                  .setGraphFunctions(true)
                                                  .setCounterBasedCreateFunctions(true);
    L0Context l0_ctx{extensionProperties};

    ze_kernel_handle_t kernel_add_const{};
    ze_kernel_handle_t kernel_add_arrays{};
    ze_module_handle_t module_add_const{};
    ze_module_handle_t module_add_arrays{};
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, "torch_benchmark_add_element_constant.cl", "torch_benchmark_add_element_constant", kernel_add_const, module_add_const));
    ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, "torch_benchmark_elementwise_sum_2.cl", "torch_benchmark_elementwise_sum_2_float", kernel_add_arrays, module_add_arrays));

    ze_event_handle_t event_1{};
    ze_event_handle_t event_2{};
    ASSERT_TEST_RESULT_SUCCESS(create_counter_based_event(l0_ctx.l0, event_1, args.useProfiling));
    ASSERT_TEST_RESULT_SUCCESS(create_counter_based_event(l0_ctx.l0, event_2, args.useProfiling));

    const uint32_t wgc = args.workgroupCount;
    const uint32_t wgs = args.workgroupSize;
    const size_t length = wgc * wgs;
    data_type *d_a, *d_b, *d_c, *d_d;
    l0_malloc_device<data_type>(l0_ctx.l0, length, d_a);
    l0_malloc_device<data_type>(l0_ctx.l0, length, d_b);
    l0_malloc_device<data_type>(l0_ctx.l0, length, d_c);
    l0_malloc_device<data_type>(l0_ctx.l0, length, d_d);

    if (!d_a || !d_b || !d_c || !d_d) {
        return TestResult::Error;
    }

    // initialize d_a for the verification phase
    const data_type init_value = 1.0f;
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendMemoryFill(l0_ctx.cmdListImmediate_1, d_a, &init_value, sizeof(data_type), length * sizeof(data_type), nullptr, 0, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_1, UINT64_MAX));

    // submit kernels
    ze_group_count_t dispatch{static_cast<uint32_t>(wgc), 1, 1};
    ze_group_size_t groupSizes = {static_cast<uint32_t>(wgs), 1, 1};
    data_type add_element = 1.0f;
    void *args_1[] = {&d_a, &add_element, &d_b};
    void *args_2[] = {&d_b, &add_element, &d_c};
    void *args_3[] = {&d_b, &d_c, &d_d};
    auto submit_kernels = [&]() {
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_add_const, dispatch, groupSizes,
                                                                              args_1, nullptr, event_1, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_2, kernel_add_const, dispatch, groupSizes,
                                                                              args_2, nullptr, event_2, 1, &event_1));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_add_arrays, dispatch, groupSizes,
                                                                              args_3, nullptr, nullptr, 1, &event_2));
        return TestResult::Success;
    };

    // capture graph
    ze_graph_handle_t graph{};
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.graphCreate(l0_ctx.l0.context, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListBeginCaptureIntoGraph(l0_ctx.cmdListImmediate_1, graph, nullptr));
    for (size_t i = 0; i < args.kernelsPerQueue; ++i) {
        ASSERT_TEST_RESULT_SUCCESS(submit_kernels());
    }
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListEndGraphCapture(l0_ctx.cmdListImmediate_1, &graph, nullptr));

    // instantiate graph
    ze_executable_graph_handle_t executableGraph{};
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListInstantiateGraph(graph, &executableGraph, nullptr));

    // benchmarking: run captured graph
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        // Currently there is an implicit sync inside execute_graph call in SYCL API
        // so only measuring exec + sync is meaningful in terms of comparing
        // perf with SYCL
        ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListAppendGraph(l0_ctx.cmdListImmediate_3, executableGraph, nullptr, nullptr, 0, nullptr));
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_3, UINT64_MAX));

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }
    ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_3, UINT64_MAX));

    // verify result
    ASSERT_TEST_RESULT_SUCCESS(verify_result<data_type>(l0_ctx, length, d_a, d_d));

    // clean up
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event_1));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event_2));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel_add_const));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_add_const));
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel_add_arrays));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_add_arrays));
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.executableGraphDestroy(executableGraph));
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.graphDestroy(graph));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event_1));
    ASSERT_ZE_RESULT_SUCCESS(zeEventDestroy(event_2));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_a));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_b));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_c));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_d));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphMultiQueue> registerTestCase(run, Api::L0, true);
