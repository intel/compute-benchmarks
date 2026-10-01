/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAX_EU_THREAD_PAUSE 32

void __builtin_IB_eu_thread_pause(uint value);

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy_throttled(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *out2, ulong count) {
    const size_t lid = get_local_id(0);
    for (size_t base = get_group_id(0) * get_local_size(0); base < count; base += get_global_size(0)) {
        const size_t idx = base + lid;
        if (idx < count) {
            out1[idx] = in1[idx] + in2[idx];
            out2[idx] = ((volatile global float4 *)out1)[idx];
        }
        if (lid == 0) {
            __builtin_IB_eu_thread_pause(MAX_EU_THREAD_PAUSE);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}
