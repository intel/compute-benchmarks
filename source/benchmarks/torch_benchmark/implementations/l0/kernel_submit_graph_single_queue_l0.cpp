/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_graph_single_queue.h"

using data_type = float;

static TestResult run(const KernelSubmitGraphSingleQueueArguments &args, Statistics &statistics) {
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

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    const size_t length = wgc * wgs;
    DeviceMemory<data_type> d_a{l0, length};
    DeviceMemory<data_type> d_b{l0, length};
    DeviceMemory<data_type> d_c{l0, length};
    DeviceMemory<data_type> d_d{l0, length};
    DeviceMemory<data_type> d_e{l0, length};

    // create kernels
    std::string kernel_file_name_1 = "";
    std::string kernel_file_name_2 = "";
    std::string kernel_name_1 = "";
    std::string kernel_name_2 = "";
    if (args.kernelName == KernelName::Empty) {
        kernel_file_name_1 = "torch_benchmark_elementwise_sum_0.cl";
        kernel_name_1 = "elementwise_sum_0";
    } else if (args.kernelName == KernelName::Add) {
        kernel_file_name_1 = "torch_benchmark_elementwise_sum_2.cl";
        kernel_name_1 = "elementwise_sum_2_float";
    } else if (args.kernelName == KernelName::AddSequence) {
        kernel_file_name_1 = "torch_benchmark_elementwise_sum_2.cl";
        kernel_name_1 = "elementwise_sum_2_float";
        kernel_file_name_2 = "torch_benchmark_add_element_constant.cl";
        kernel_name_2 = "add_element_constant";
    } else {
        std::cerr << "Invalid kernel name argument." << std::endl;
        return TestResult::InvalidArgs;
    }
    std::unique_ptr<Kernel> kernel_1{nullptr};
    std::unique_ptr<Kernel> kernel_2{nullptr};
    if (!kernel_file_name_1.empty() && !kernel_name_1.empty()) {
        kernel_1 = std::make_unique<Kernel>(l0, kernel_file_name_1, kernel_name_1);
    }
    if (!kernel_file_name_2.empty() && !kernel_name_2.empty()) {
        kernel_2 = std::make_unique<Kernel>(l0, kernel_file_name_2, kernel_name_2);
    }
    ze_group_count_t dispatch{wgc, 1, 1};
    ze_group_size_t groupSizes{wgs, 1, 1};
    data_type add_element_1 = 2.0f;
    data_type add_element_2 = 1.0f;
    void *args_1[] = {d_c.getAddress(), d_a.getAddress(), d_b.getAddress()};
    void *args_2[] = {d_d.getAddress(), d_c.getAddress(), &add_element_1};
    void *args_3[] = {d_e.getAddress(), d_d.getAddress(), &add_element_2};

    auto submit_kernels = [&]() -> TestResult {
        if (args.kernelName == KernelName::Empty) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_1->get(), dispatch, groupSizes,
                                                                                  nullptr, nullptr, nullptr, 0, nullptr));
        } else if (args.kernelName == KernelName::Add) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_1->get(), dispatch, groupSizes,
                                                                                  args_1, nullptr, nullptr, 0, nullptr));
        } else if (args.kernelName == KernelName::AddSequence) {
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_1->get(), dispatch, groupSizes,
                                                                                  args_1, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_2->get(), dispatch, groupSizes,
                                                                                  args_2, nullptr, nullptr, 0, nullptr));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmd_list_1.get(), kernel_2->get(), dispatch, groupSizes,
                                                                                  args_3, nullptr, nullptr, 0, nullptr));
        }
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
        for (size_t j = 0; j < args.kernelBatchSize; ++j) {
            ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmd_list_2.get(), graph.getExecutable(), nullptr, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmd_list_2.get(), UINT64_MAX));

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphSingleQueue> registerTestCase(run, Api::L0, true);
