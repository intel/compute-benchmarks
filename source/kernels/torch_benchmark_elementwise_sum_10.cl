/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define KERNEL_SUM_10(DATA_TYPE) \
__kernel void elementwise_sum_10_##DATA_TYPE(__global const DATA_TYPE *src0, \
                                            __global const DATA_TYPE *src1, \
                                            __global const DATA_TYPE *src2, \
                                            __global const DATA_TYPE *src3, \
                                            __global const DATA_TYPE *src4, \
                                            __global const DATA_TYPE *src5, \
                                            __global const DATA_TYPE *src6, \
                                            __global const DATA_TYPE *src7, \
                                            __global const DATA_TYPE *src8, \
                                            __global const DATA_TYPE *src9, \
                                            __global DATA_TYPE *dst) { \
    int gid = get_global_id(0); \
    dst[gid] = src0[gid] + src1[gid] + src2[gid] + src3[gid] + src4[gid] + src5[gid] + src6[gid] + src7[gid] + src8[gid] + src9[gid]; \
}

KERNEL_SUM_10(double)
KERNEL_SUM_10(float)
KERNEL_SUM_10(int)
