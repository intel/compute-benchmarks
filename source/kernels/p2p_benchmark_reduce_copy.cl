/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

enum LSC_LDCC {
    LSC_LDCC_L1UC_L3C = 2
};
enum LSC_STCC {
    LSC_STCC_L1WB_L3WB = 7
};
uint4 __builtin_IB_lsc_load_global_uint4(const global uint4 *base, int immElemOff, enum LSC_LDCC cacheOpt);
void __builtin_IB_lsc_store_global_uint4(global uint4 *base, int immElemOff, uint4 val, enum LSC_STCC cacheOpt);

__attribute__((intel_reqd_sub_group_size(16))) kernel void reduce_copy(global const float4 *in1, global const float4 *in2, global float4 *out1, global float4 *out2, ulong count) {
    const size_t idx = get_global_id(0);
    if (idx < count) {
        out1[idx] = in1[idx] + in2[idx];
        const uint4 value = __builtin_IB_lsc_load_global_uint4((const global uint4 *)out1 + idx, 0, LSC_LDCC_L1UC_L3C);
        __builtin_IB_lsc_store_global_uint4((global uint4 *)out2 + idx, 0, value, LSC_STCC_L1WB_L3WB);
    }
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void copy(global const float4 *src, global float4 *dst, ulong count) {
    const size_t idx = get_global_id(0);
    if (idx < count) {
        const uint4 value = __builtin_IB_lsc_load_global_uint4((const global uint4 *)src + idx, 0, LSC_LDCC_L1UC_L3C);
        __builtin_IB_lsc_store_global_uint4((global uint4 *)dst + idx, 0, value, LSC_STCC_L1WB_L3WB);
    }
}

__attribute__((intel_reqd_sub_group_size(16))) kernel void p2p_barrier(global atomic_uint *sync, global uint *peerSync) {
    if (get_global_id(0) == 0) {
        const uint count = atomic_fetch_add_explicit(&sync[0], 1u, memory_order_relaxed, memory_scope_device) + 1u;
        atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release, memory_scope_all_svm_devices);
        ((volatile global uint *)peerSync)[1] = count;
        atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_release, memory_scope_all_svm_devices);
        while (atomic_load_explicit(&sync[1], memory_order_relaxed, memory_scope_all_svm_devices) < count) {
        }
        atomic_work_item_fence(CLK_GLOBAL_MEM_FENCE, memory_order_acquire, memory_scope_all_svm_devices);
    }
    barrier(CLK_GLOBAL_MEM_FENCE);
}
