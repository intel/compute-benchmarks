/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "common.hpp"
#include "definitions/kernel_submit_graph_vllm_mock.h"

using data_type = float;

constexpr uint32_t BATCH_SIZE = 1;

static TestResult run(const KernelSubmitGraphVllmMockArguments &args, Statistics &statistics) {
    ComboProfilerWithStats profiler(Configuration::get().profilerType);

    if (isNoopRun()) {
        profiler.pushNoop(statistics);
        return TestResult::Nooped;
    }

    // validate arguments
    if (args.graphScenario > 3) {
        std::cerr << "Invalid graph scenario: " << args.graphScenario << std::endl;
        return TestResult::InvalidArgs;
    }

    // setup
    ExtensionProperties extensionProperties = ExtensionProperties::create()
                                                  .setGraphFunctions(true);
    LevelZero l0{extensionProperties};
    // note: Only in-order queues are supported in this benchmark
    CommandList cmdListImmediate{l0.context, l0.device, zeDefaultGPUImmediateCommandQueueDesc};

    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    size_t input_len = args.allocCount * BATCH_SIZE;

    // envs taken from config
    const size_t vocab_size = 128256;
    size_t hidden_size = 2048;
    const int head_dim = 64;
    const int num_attention_heads = 32;
    const int num_key_value_heads = 8;
    const int num_hidden_layers = 16;
    const int max_position_embeddings = 131072;
    int intermediate_size = 8192;
    // envs created from vllm config
    const int num_blocks = 65507;
    const int block_size = 16;
    // other envs
    const int block_table_size = 8192;
    size_t xnumel_0 = input_len * num_attention_heads * head_dim;
    size_t xnumel_1 = input_len * num_key_value_heads * head_dim;
    size_t xnumel_2 = input_len * intermediate_size;

    // variables used for graph 1
    DeviceMemory<int> input_token_id{l0, input_len};
    DeviceMemory<data_type> embedding_table{l0, vocab_size * hidden_size};
    DeviceMemory<data_type> input_layernorm{l0, hidden_size};
    DeviceMemory<data_type> fused_embedding_layernorm_output{l0, input_len * hidden_size};
    DeviceMemory<data_type> residual{l0, input_len * hidden_size};
    DeviceMemory<unsigned char> qkv_weight{l0, hidden_size * head_dim * (num_attention_heads + 2 * num_key_value_heads)};
    DeviceMemory<long> position_ids{l0, input_len};
    DeviceMemory<data_type> linear_qkv_output{l0, input_len * head_dim * (num_attention_heads + 2 * num_key_value_heads)};
    DeviceMemory<data_type> rope_embedding_weight{l0, max_position_embeddings * head_dim};
    DeviceMemory<data_type> q_rope_output{l0, xnumel_0};
    DeviceMemory<data_type> k_rope_output{l0, xnumel_1};

    // variables used for kernel reshape_and_cache
    // value is splitted from linear_qkv_output(llama.py LlamaAttention)
    DeviceMemory<data_type> v{l0, xnumel_1};
    DeviceMemory<data_type> key_cache{l0, num_blocks * block_size * num_key_value_heads * head_dim};
    DeviceMemory<data_type> value_cache{l0, num_blocks * block_size * num_key_value_heads * head_dim};
    DeviceMemory<long> slot_mapping{l0, input_len};
    DeviceMemory<data_type> k_scale{l0, 1};
    DeviceMemory<data_type> v_scale{l0, 1};

    // variables used for kernel flash_attn
    DeviceMemory<data_type> flash_attn_output{l0, xnumel_0};
    DeviceMemory<int> cu_seqlens_q{l0, BATCH_SIZE + 1};
    DeviceMemory<int> cu_seqlens_k{l0, BATCH_SIZE + 1};
    DeviceMemory<int> seqused_k{l0, BATCH_SIZE};
    DeviceMemory<int> block_table{l0, BATCH_SIZE * block_table_size};
    float attn_dropout = 0.0f;
    float softmax_scale = 0.125f;
    bool causal = false;
    bool return_softmax = true;
    int block_start = -1;
    int block_end = -1;
    float attn_bias = 0.0f;
    bool use_alibi = false;

    // variables used for graph 2
    DeviceMemory<data_type> o_proj_output{l0, input_len * hidden_size};
    DeviceMemory<data_type> post_attention_layernorm{l0, hidden_size};
    DeviceMemory<data_type> layernorm_output{l0, input_len * hidden_size};
    DeviceMemory<data_type> up_project_gate_output{l0, input_len * intermediate_size * 2};
    DeviceMemory<data_type> fused_silu_slice_output{l0, input_len * intermediate_size};
    DeviceMemory<data_type> down_project_output{l0, input_len * hidden_size};

    // input_layernorm: reuse the variables from graph 1
    DeviceMemory<data_type> fused_embedding_layernorm_output_2{l0, input_len * hidden_size};
    DeviceMemory<data_type> residual_2{l0, input_len * hidden_size};

    // create kernels
    ze_group_count_t dispatch{wgc, 1, 1};
    ze_group_size_t groupSizes{wgs, 1, 1};
    std::string kernelFileName = "torch_benchmark_vllm_mock.cl";
    std::string kernelName = "mock_triton_red_fused__to_copy_add_embedding_mean_mul_pow_rsqrt_0";
    Kernel kernel_fused_embedding_layernorm{l0, kernelFileName, kernelName};
    kernelName = "mock_reshape_and_cache";
    Kernel kernel_reshape_and_cache{l0, kernelFileName, kernelName};
    kernelName = "mock_flash_attn";
    Kernel kernel_flash_attn{l0, kernelFileName, kernelName};
    kernelName = "mock_triton_poi_fused_1_3";
    Kernel kernel_triton_poi_fused_1_3{l0, kernelFileName, kernelName};
    kernelName = "mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0";
    Kernel kernel_fused_between_attn_mlp{l0, kernelFileName, kernelName};
    kernelName = "mock_triton_poi_fused_mul_silu_slice_1";
    Kernel kernel_fused_silu_slice{l0, kernelFileName, kernelName};
    kernelName = "mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2";
    Kernel kernel_fused_between_mlp_qkvlinear{l0, kernelFileName, kernelName};
    kernelName = "mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer";
    Kernel kernel_fused_layernorm_last{l0, kernelFileName, kernelName};

    auto submit_kernel = [&](ze_command_list_handle_t cmdList, ze_kernel_handle_t kernel, auto... args) {
        void *kernelArguments[] = {args...};
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListAppendLaunchKernelWithArguments(cmdList, kernel, dispatch, groupSizes,
                                                                              kernelArguments, nullptr, nullptr, 0, nullptr));
        return TestResult::Success;
    };

    // create graphs
    Graph graph_1{l0}, graph_2{l0}, graph_3{l0};

    int XBLOCK_1 = 1;
    int XBLOCK_2 = 1024;
    int R0_BLOCK = 2048;

    if (args.graphScenario == 0 || args.graphScenario == 1) {
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListBeginCaptureIntoGraph(cmdListImmediate.get(), graph_1.get(), nullptr));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_embedding_layernorm.get(),
                                                 input_token_id.getAddress(), embedding_table.getAddress(), input_layernorm.getAddress(),
                                                 fused_embedding_layernorm_output.getAddress(), residual.getAddress(),
                                                 &input_len, &hidden_size, &XBLOCK_1, &R0_BLOCK));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_triton_poi_fused_1_3.get(),
                                                 linear_qkv_output.getAddress(), position_ids.getAddress(), rope_embedding_weight.getAddress(), q_rope_output.getAddress(), k_rope_output.getAddress(),
                                                 &xnumel_0, &xnumel_1, &XBLOCK_2));
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListEndGraphCapture(cmdListImmediate.get(), graph_1.getAddress(), nullptr));
        graph_1.instantiate();
    }
    if (args.graphScenario == 0 || args.graphScenario == 2) {
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListBeginCaptureIntoGraph(cmdListImmediate.get(), graph_2.get(), nullptr));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_between_attn_mlp.get(),
                                                 o_proj_output.getAddress(), residual.getAddress(), post_attention_layernorm.getAddress(), layernorm_output.getAddress(), &input_len, &hidden_size, &XBLOCK_1, &R0_BLOCK));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_silu_slice.get(),
                                                 up_project_gate_output.getAddress(), fused_silu_slice_output.getAddress(), &xnumel_2, &XBLOCK_2));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_between_mlp_qkvlinear.get(),
                                                 down_project_output.getAddress(), fused_embedding_layernorm_output.getAddress(), residual.getAddress(), input_layernorm.getAddress(),
                                                 fused_embedding_layernorm_output_2.getAddress(), residual_2.getAddress(), &input_len, &hidden_size, &XBLOCK_1, &R0_BLOCK));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_triton_poi_fused_1_3.get(),
                                                 linear_qkv_output.getAddress(), position_ids.getAddress(), rope_embedding_weight.getAddress(), q_rope_output.getAddress(), k_rope_output.getAddress(),
                                                 &xnumel_0, &xnumel_1, &XBLOCK_2));
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListEndGraphCapture(cmdListImmediate.get(), graph_2.getAddress(), nullptr));
        graph_2.instantiate();
    }
    if (args.graphScenario == 0 || args.graphScenario == 3) {
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListBeginCaptureIntoGraph(cmdListImmediate.get(), graph_3.get(), nullptr));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_between_attn_mlp.get(),
                                                 o_proj_output.getAddress(), residual.getAddress(), post_attention_layernorm.getAddress(), layernorm_output.getAddress(), &input_len, &hidden_size, &XBLOCK_1, &R0_BLOCK));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_silu_slice.get(),
                                                 up_project_gate_output.getAddress(), fused_silu_slice_output.getAddress(), &xnumel_2, &XBLOCK_2));
        ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_fused_layernorm_last.get(),
                                                 down_project_output.getAddress(), fused_embedding_layernorm_output.getAddress(), residual.getAddress(), input_layernorm.getAddress(), &input_len, &hidden_size, &XBLOCK_1, &R0_BLOCK));
        ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListEndGraphCapture(cmdListImmediate.get(), graph_3.getAddress(), nullptr));
        graph_3.instantiate();
    }

    // benchmark
    // Currently there is an implicit sync inside execute_graph call in SYCL API
    // so only measuring exec + sync is meaningful in terms of comparing
    // perf with SYCL
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        /* graph scenario 0 is a combined graph execution of all graphs of Llama-3.2-1B without the gemm kernel:
        *      graph 1
        *      for j in 1...num_hidden_layers:
        *           kernel reshape_and_cache
        *           kernel flash_attn
        *           graph 2
        *      kernel reshape_and_cache
        *      kernel flash_attn
        *      graph 3

        * graph scenarios 1-3 are executing graph 1-3 separately:
        *      graph 1 (layer normalization): fused_embedding_layernorm + triton_poi_fused_1_3
        *      graph 2 (self-attention layer): fused_between_attn_mlp + fused_silu_slice + fused_between_mlp_qkvlinear + triton_poi_fused_1_3
        *      graph 3 (post attention layer normalization): fused_between_attn_mlp + fused_silu_slice + fused_layernorm_last
        */
        if (args.graphScenario == 0) {
            ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmdListImmediate.get(), graph_1.getExecutable(), nullptr, nullptr, 0, nullptr));
            for (int j = 1; j < num_hidden_layers; j++) {
                ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_reshape_and_cache.get(),
                                                         k_rope_output.getAddress(), v.getAddress(), key_cache.getAddress(), value_cache.getAddress(), slot_mapping.getAddress(), k_scale.getAddress(), v_scale.getAddress()));
                ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_flash_attn.get(),
                                                         q_rope_output.getAddress(), key_cache.getAddress(), value_cache.getAddress(), flash_attn_output.getAddress(), cu_seqlens_q.getAddress(), cu_seqlens_k.getAddress(), seqused_k.getAddress(), block_table.getAddress(),
                                                         &input_len, &input_len, &attn_dropout, &softmax_scale, &causal, &return_softmax, &block_start, &block_end, &attn_bias, &use_alibi));
                ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate.get(), UINT64_MAX));
                ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmdListImmediate.get(), graph_2.getExecutable(), nullptr, nullptr, 0, nullptr));
            }
            ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_reshape_and_cache.get(),
                                                     k_rope_output.getAddress(), v.getAddress(), key_cache.getAddress(), value_cache.getAddress(), slot_mapping.getAddress(), k_scale.getAddress(), v_scale.getAddress()));
            ASSERT_TEST_RESULT_SUCCESS(submit_kernel(cmdListImmediate.get(), kernel_flash_attn.get(),
                                                     q_rope_output.getAddress(), key_cache.getAddress(), value_cache.getAddress(), flash_attn_output.getAddress(), cu_seqlens_q.getAddress(), cu_seqlens_k.getAddress(), seqused_k.getAddress(), block_table.getAddress(),
                                                     &input_len, &input_len, &attn_dropout, &softmax_scale, &causal, &return_softmax, &block_start, &block_end, &attn_bias, &use_alibi));
            ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate.get(), UINT64_MAX));
            ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmdListImmediate.get(), graph_3.getExecutable(), nullptr, nullptr, 0, nullptr));
        } else if (args.graphScenario == 1) {
            ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmdListImmediate.get(), graph_1.getExecutable(), nullptr, nullptr, 0, nullptr));
        } else if (args.graphScenario == 2) {
            ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmdListImmediate.get(), graph_2.getExecutable(), nullptr, nullptr, 0, nullptr));
        } else if (args.graphScenario == 3) {
            ASSERT_ZE_RESULT_SUCCESS(l0.graphExtension.commandListAppendGraph(cmdListImmediate.get(), graph_3.getExecutable(), nullptr, nullptr, 0, nullptr));
        }
        ASSERT_ZE_RESULT_SUCCESS(zeCommandListHostSynchronize(cmdListImmediate.get(), UINT64_MAX));

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphVllmMock> registerTestCase(run, Api::L0, true);
