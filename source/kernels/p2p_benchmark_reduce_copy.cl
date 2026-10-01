/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *out2) {
    const size_t idx = get_global_id(0);
    out1[idx] = in1[idx] + in2[idx];
    // oneCCL reads out1 back from memory before the peer store, volatile keeps that load
    out2[idx] = ((volatile global float4 *)out1)[idx];
}
