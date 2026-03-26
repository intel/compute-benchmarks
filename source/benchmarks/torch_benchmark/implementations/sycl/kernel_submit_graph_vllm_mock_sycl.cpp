/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "common.hpp"
#include "definitions/graph_vllm_kernels.h"
#include "definitions/kernel_submit_graph_vllm_mock.h"

using data_type = float;

namespace syclex = sycl::ext::oneapi::experimental;
using graph_exec = syclex::command_graph<syclex::graph_state::executable>;

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
    // Note: Only in-order queues are supported in this benchmark
    bool useOOQ = false;
    Sycl sycl = args.useProfiling
                    ? Sycl{useOOQ, sycl::property::queue::enable_profiling()}
                    : Sycl{useOOQ};
    const uint32_t wgc = args.kernelWGCount;
    const uint32_t wgs = args.kernelWGSize;
    const uint32_t input_len = args.allocCount * BATCH_SIZE;

    // envs taken from config
    const int vocab_size = 128256;
    const int hidden_size = 2048;
    const int head_dim = 64;
    const int num_attention_heads = 32;
    const int num_key_value_heads = 8;
    const int num_hidden_layers = 16;
    const int max_position_embeddings = 131072;
    const int intermediate_size = 8192;
    // envs created from vllm config
    const int num_blocks = 65507;
    const int block_size = 16;
    // other envs
    const int block_table_size = 8192;
    int xnumel_0 = input_len * num_attention_heads * head_dim;
    int xnumel_1 = input_len * num_key_value_heads * head_dim;

    // variables used for graph 1
    auto input_token_id = make_device_ptr<int>(sycl, input_len);
    auto embedding_table = make_device_ptr<data_type>(sycl, vocab_size * hidden_size);
    auto input_layernorm = make_device_ptr<data_type>(sycl, hidden_size);
    auto fused_embedding_layernorm_output = make_device_ptr<data_type>(sycl, input_len * hidden_size);
    auto residual = make_device_ptr<data_type>(sycl, input_len * hidden_size);
    auto qkv_weight = make_device_ptr<unsigned char>(sycl, hidden_size * head_dim * (num_attention_heads + 2 * num_key_value_heads));
    auto position_ids = make_device_ptr<long>(sycl, input_len);
    auto linear_qkv_output = make_device_ptr<data_type>(sycl, input_len * head_dim * (num_attention_heads + 2 * num_key_value_heads));
    auto rope_embedding_weight = make_device_ptr<data_type>(sycl, max_position_embeddings * head_dim);

    auto q_rope_output = make_device_ptr<data_type>(sycl, xnumel_0);
    auto k_rope_output = make_device_ptr<data_type>(sycl, xnumel_1);

    // variables used for kernel reshape_and_cache
    // value is splitted from linear_qkv_output(llama.py LlamaAttention)
    auto v = make_device_ptr<data_type>(sycl, xnumel_1);
    auto key_cache = make_device_ptr<data_type>(sycl, num_blocks * block_size * num_key_value_heads * head_dim);
    auto value_cache = make_device_ptr<data_type>(sycl, num_blocks * block_size * num_key_value_heads * head_dim);
    auto slot_mapping = make_device_ptr<long>(sycl, input_len);
    auto k_scale = make_device_ptr<data_type>(sycl, 1);
    auto v_scale = make_device_ptr<data_type>(sycl, 1);

    // variables used for kernel flash_attn
    auto flash_attn_output = make_device_ptr<data_type>(sycl, xnumel_0);
    auto cu_seqlens_q = make_device_ptr<int>(sycl, BATCH_SIZE + 1);
    auto cu_seqlens_k = make_device_ptr<int>(sycl, BATCH_SIZE + 1);
    auto seqused_k = make_device_ptr<int>(sycl, BATCH_SIZE);
    auto block_table = make_device_ptr<int>(sycl, BATCH_SIZE * block_table_size);

    // variables used for graph 2
    auto o_proj_output = make_device_ptr<data_type>(sycl, input_len * hidden_size);
    auto post_attention_layernorm = make_device_ptr<data_type>(sycl, hidden_size);
    auto layernorm_output = make_device_ptr<data_type>(sycl, input_len * hidden_size);
    auto up_project_gate_output = make_device_ptr<data_type>(sycl, input_len * intermediate_size * 2);
    auto fused_silu_slice_output = make_device_ptr<data_type>(sycl, input_len * intermediate_size);
    auto down_project_output = make_device_ptr<data_type>(sycl, input_len * hidden_size);

    // input_layernorm: reuse the variables from graph 1
    auto fused_embedding_layernorm_output_2 = make_device_ptr<data_type>(sycl, input_len * hidden_size);
    auto residual_2 = make_device_ptr<data_type>(sycl, input_len * hidden_size);

    auto graph_1 = syclex::command_graph<syclex::graph_state::modifiable>(sycl.queue.get_context(), sycl.queue.get_device());
    auto graph_2 = syclex::command_graph<syclex::graph_state::modifiable>(sycl.queue.get_context(), sycl.queue.get_device());
    auto graph_3 = syclex::command_graph<syclex::graph_state::modifiable>(sycl.queue.get_context(), sycl.queue.get_device());
    std::unique_ptr<graph_exec> graph_exec_1, graph_exec_2, graph_exec_3;

    int XBLOCK_1 = 1;
    int XBLOCK_2 = 1024;
    int R0_BLOCK = 2048;

    if (args.graphScenario == 0 || args.graphScenario == 1) {
        // capture graph 1
        graph_1.begin_recording(sycl.queue);
        submit_kernel_fused_embedding_layernorm<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                           input_token_id.get(), embedding_table.get(), input_layernorm.get(), fused_embedding_layernorm_output.get(), residual.get(), input_len, hidden_size, XBLOCK_1, R0_BLOCK);
        submit_kernel_mock_triton_poi_fused_1_3<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                           linear_qkv_output.get(), position_ids.get(), rope_embedding_weight.get(), q_rope_output.get(), k_rope_output.get(),
                                                           xnumel_0, xnumel_1, XBLOCK_2);
        graph_1.end_recording();
        graph_exec_1 = std::make_unique<graph_exec>(graph_1.finalize());
    }
    if (args.graphScenario == 0 || args.graphScenario == 2) {
        // capture graph 2
        graph_2.begin_recording(sycl.queue);
        submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                                         o_proj_output.get(), residual.get(), post_attention_layernorm.get(), layernorm_output.get(), input_len, hidden_size, XBLOCK_1, R0_BLOCK);
        submit_kernel_mock_triton_poi_fused_mul_silu_slice_1<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                        up_project_gate_output.get(), fused_silu_slice_output.get(), input_len * intermediate_size, XBLOCK_2);
        submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                                         down_project_output.get(), fused_embedding_layernorm_output.get(), residual.get(), input_layernorm.get(),
                                                                                         fused_embedding_layernorm_output_2.get(), residual_2.get(), input_len, hidden_size, XBLOCK_1, R0_BLOCK);
        submit_kernel_mock_triton_poi_fused_1_3<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                           linear_qkv_output.get(), position_ids.get(), rope_embedding_weight.get(), q_rope_output.get(), k_rope_output.get(),
                                                           xnumel_0, xnumel_1, XBLOCK_2);
        graph_2.end_recording();
        graph_exec_2 = std::make_unique<graph_exec>(graph_2.finalize());
    }
    if (args.graphScenario == 0 || args.graphScenario == 3) {
        // capture graph 3
        graph_3.begin_recording(sycl.queue);
        submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                                         o_proj_output.get(), residual.get(), post_attention_layernorm.get(), layernorm_output.get(), input_len, hidden_size, XBLOCK_1, R0_BLOCK);
        submit_kernel_mock_triton_poi_fused_mul_silu_slice_1<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                        up_project_gate_output.get(), fused_silu_slice_output.get(), input_len * intermediate_size, XBLOCK_2);
        submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                                                    down_project_output.get(), fused_embedding_layernorm_output.get(), residual.get(), input_layernorm.get(), input_len, hidden_size, XBLOCK_1, R0_BLOCK);
        graph_3.end_recording();
        graph_exec_3 = std::make_unique<graph_exec>(graph_3.finalize());
    }

    // benchmark
    // Currently there is an implicit sync inside execute_graph call
    // so only measuring exec + sync is meaningful in terms of comparing
    // perf with L0
    for (size_t i = 0; i < args.iterations; ++i) {
        profiler.measureStart();

        /* graph scenario 0 is a combined graph execution of all graphs of Llama-3.2-1B without the gemm kernel
         * graph_1
         * for i in 1...num_hidden_layers:
         *      launch_kernel_reshape_and_cache
         *      launch_kernel_flash_attn
         *      graph 2
         * launch_kernel_reshape_and_cache
         * launch_kernel_flash_attn
         * graph 3
         *
         * graph scenarios 1-3 are executing graph 1-3 separately:
         *      graph 1 (layer normalization): fused_embedding_layernorm + triton_poi_fused_1_3
         *      graph 2 (self-attention layer): fused_between_attn_mlp + fused_silu_slice + fused_between_mlp_qkvlinear + triton_poi_fused_1_3
         *      graph 3 (post attention layer normalization): fused_between_attn_mlp + fused_silu_slice + fused_layernorm_last
         */
        if (args.graphScenario == 0) {
            syclex::execute_graph(sycl.queue, *graph_exec_1);
            for (int j = 0; j < num_hidden_layers - 1; ++j) {
                submit_kernel_mock_reshape_and_cache<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                                k_rope_output.get(), v.get(), key_cache.get(), value_cache.get(), slot_mapping.get(), k_scale.get(), v_scale.get());
                submit_kernel_mock_flash_attn<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                         q_rope_output.get(), key_cache.get(), value_cache.get(), flash_attn_output.get(), cu_seqlens_q.get(), cu_seqlens_k.get(), seqused_k.get(), block_table.get(),
                                                         input_len, input_len, 0.0f, 0.125f, false, true, -1, -1, 0.0f, false);
                sycl.queue.wait();
                syclex::execute_graph(sycl.queue, *graph_exec_2);
            }
            submit_kernel_mock_reshape_and_cache<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                            k_rope_output.get(), v.get(), key_cache.get(), value_cache.get(), slot_mapping.get(), k_scale.get(), v_scale.get());
            submit_kernel_mock_flash_attn<data_type>(wgc, wgs, sycl.queue, args.useEvents,
                                                     q_rope_output.get(), key_cache.get(), value_cache.get(), flash_attn_output.get(), cu_seqlens_q.get(), cu_seqlens_k.get(), seqused_k.get(), block_table.get(),
                                                     input_len, input_len, 0.0f, 0.125f, false, true, -1, -1, 0.0f, false);
            sycl.queue.wait();
            syclex::execute_graph(sycl.queue, *graph_exec_3);
        } else if (args.graphScenario == 1) {
            syclex::execute_graph(sycl.queue, *graph_exec_1);
        } else if (args.graphScenario == 2) {
            syclex::execute_graph(sycl.queue, *graph_exec_2);
        } else if (args.graphScenario == 3) {
            syclex::execute_graph(sycl.queue, *graph_exec_3);
        }
        sycl.queue.wait();

        profiler.measureEnd();
        profiler.pushStats(statistics);
    }

    return TestResult::Success;
}

[[maybe_unused]] static RegisterTestCaseImplementation<KernelSubmitGraphVllmMock> registerTestCase(run, Api::SYCL);
