/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "definitions/kernel_submit_graph_single_queue.h"
#include "kernel_submit_common.hpp"

using data_type = float;

static TestResult run(const KernelSubmitGraphSingleQueueArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        std::cerr << "Noop run for Torch L0 benchmark" << std::endl;
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setGraphFunctions(true);
    // Note: Only in-order queues are supported in this benchmark
    bool useInOrderQueue = true;
    L0Context l0_ctx{extensionProperties, useInOrderQueue};

    ze_kernel_handle_t kernel_1{};
    ze_kernel_handle_t kernel_2{};
    ze_module_handle_t module_1{};
    ze_module_handle_t module_2{};
    if (args.kernelName == KernelName::Empty) {
        ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, "torch_benchmark_elementwise_sum_0.cl", "elementwise_sum_0", kernel_1, module_1));
    } else if (args.kernelName == KernelName::Add) {
        ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, "torch_benchmark_elementwise_sum_2.cl", "torch_benchmark_elementwise_sum_2_float", kernel_1, module_1));
    } else if (args.kernelName == KernelName::AddSequence) {
        ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, "torch_benchmark_elementwise_sum_2.cl", "torch_benchmark_elementwise_sum_2_float", kernel_1, module_1));
        ASSERT_TEST_RESULT_SUCCESS(create_kernel(l0_ctx.l0, "torch_benchmark_add_element_constant.cl", "torch_benchmark_add_element_constant", kernel_2, module_2));
    } else {
        return TestResult::InvalidArgs;
    }

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    const size_t length = wgc * wgs;
    data_type *d_a, *d_b, *d_c, *d_d, *d_e;
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0_ctx.l0, length, d_a));
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0_ctx.l0, length, d_b));
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0_ctx.l0, length, d_c));
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0_ctx.l0, length, d_d));
    ASSERT_TEST_RESULT_SUCCESS(l0_malloc_device<data_type>(l0_ctx.l0, length, d_e));

    auto submit_kernels = [&]() {
        ze_group_count_t dispatch{static_cast<uint32_t>(wgc), 1, 1};
        ze_group_size_t groupSizes = {static_cast<uint32_t>(wgs), 1, 1};

        if (args.kernelName == KernelName::Empty) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_1, dispatch, groupSizes,
                                                                                  nullptr, nullptr, nullptr, 0, nullptr));
        } else if (args.kernelName == KernelName::Add) {
            void *args_1[] = {&d_a, &d_b, &d_c};
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_1, dispatch, groupSizes,
                                                                                  args_1, nullptr, nullptr, 0, nullptr));
        } else if (args.kernelName == KernelName::AddSequence) {
            data_type add_element_1 = 2.0f;
            data_type add_element_2 = 1.0f;
            void *args_1[] = {&d_a, &d_b, &d_c};
            void *args_2[] = {&d_c, &add_element_1, &d_d};
            void *args_3[] = {&d_d, &add_element_2, &d_e};
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_1, dispatch, groupSizes,
                                                                                  args_1, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_2, dispatch, groupSizes,
                                                                                  args_2, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(l0_ctx.cmdListImmediate_1, kernel_2, dispatch, groupSizes,
                                                                                  args_3, nullptr, nullptr, 0, nullptr));
        } else {
            return TestResult::Error;
        }
        return TestResult::Success;
    };

    // capture graph
    ze_graph_handle_t graph{};
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.graphCreate(l0_ctx.l0.context, &graph, nullptr));
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListBeginCaptureIntoGraph(l0_ctx.cmdListImmediate_1, graph, nullptr));
    for (size_t i = 0; i < args.kernelGroupsCount; ++i) {
        ASSERT_TEST_RESULT_SUCCESS(submit_kernels());
    }
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListEndGraphCapture(l0_ctx.cmdListImmediate_1, &graph, nullptr));

    // instantiate graph
    ze_executable_graph_handle_t executableGraph{};
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListInstantiateGraph(graph, &executableGraph, nullptr));

    // run captured graph
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        // Currently there is an implicit sync inside execute_graph call in SYCL API
        // so only measuring exec + sync is meaningful in terms of comparing
        // perf with SYCL
        for (size_t j = 0; j < args.kernelBatchSize; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.commandListAppendGraph(l0_ctx.cmdListImmediate_2, executableGraph, nullptr, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(l0_ctx.cmdListImmediate_2, UINT64_MAX));

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    // clean up
    ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel_1));
    ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_1));
    if (args.kernelName == KernelName::AddSequence) {
        ASSERT_ZE_RESULT_SUCCESS(zeKernelDestroy(kernel_2));
        ASSERT_ZE_RESULT_SUCCESS(zeModuleDestroy(module_2));
    }
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.executableGraphDestroy(executableGraph));
    ASSERT_ZE_RESULT_SUCCESS(l0_ctx.l0.graphExtension.graphDestroy(graph));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_a));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_b));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_c));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_d));
    ASSERT_ZE_RESULT_SUCCESS(zeMemFree(l0_ctx.l0.context, d_e));

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphSingleQueue> registerTestCase(run, Api::L0, true);
