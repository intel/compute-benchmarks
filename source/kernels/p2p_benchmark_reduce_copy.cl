/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *out2) {
    const size_t idx = get_global_id(0);
    out1[idx] = in1[idx] + in2[idx];
    out2[idx] = ((volatile global float4 *)out1)[idx];
}
