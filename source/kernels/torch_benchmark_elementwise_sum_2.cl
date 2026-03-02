/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define KERNEL_SUM_2(DATA_TYPE) \
__kernel void elementwise_sum_2_##DATA_TYPE(__global DATA_TYPE *dest, \
                                            __global const DATA_TYPE *src0, \
                                            __global const DATA_TYPE *src1) { \
    int gid = get_global_id(0); \
    dest[gid] = src0[gid] + src1[gid]; \
}

KERNEL_SUM_2(float)
KERNEL_SUM_2(int)
