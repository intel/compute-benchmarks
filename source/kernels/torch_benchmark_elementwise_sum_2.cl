/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define KERNEL_SUM_2(DATA_TYPE) \
__kernel void torch_benchmark_elementwise_sum_2_##DATA_TYPE(__global const DATA_TYPE *src0, \
                                                            __global const DATA_TYPE *src1, \
                                                            __global DATA_TYPE *dst) { \
    int gid = get_global_id(0); \
    dst[gid] = src0[gid] + src1[gid]; \
}

KERNEL_SUM_2(float)
KERNEL_SUM_2(int)
