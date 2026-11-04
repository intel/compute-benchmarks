/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAX_EU_THREAD_PAUSE 32

void __attribute__((overloadable)) intel_eu_thread_pause(uint value);

__attribute__((intel_reqd_sub_group_size(16))) kernel void copy_throttled(global const float4 *source, global float4 *destination, ulong count) {
    const size_t lid = get_local_id(0);
    for (size_t base = get_group_id(0) * get_local_size(0); base < count; base += get_global_size(0)) {
        const size_t idx = base + lid;
        if (idx < count) {
            destination[idx] = source[idx];
        }
        if (lid == 0) {
            intel_eu_thread_pause(MAX_EU_THREAD_PAUSE);
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}
