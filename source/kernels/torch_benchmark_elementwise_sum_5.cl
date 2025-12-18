/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define KERNEL_SUM_5(DATA_TYPE) \
__kernel void elementwise_sum_5_##DATA_TYPE(__global const DATA_TYPE *src0, \
                                            __global const DATA_TYPE *src1, \
                                            __global const DATA_TYPE *src2, \
                                            __global const DATA_TYPE *src3, \
                                            __global const DATA_TYPE *src4, \
                                            __global DATA_TYPE *dst) { \
    int gid = get_global_id(0); \
    dst[gid] = src0[gid] + src1[gid] + src2[gid] + src3[gid] + src4[gid]; \
}

KERNEL_SUM_5(double)
KERNEL_SUM_5(float)
KERNEL_SUM_5(int)
