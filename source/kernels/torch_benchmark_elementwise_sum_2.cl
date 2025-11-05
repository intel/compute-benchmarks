/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void elementwise_sum_2(__global const int *src0,
                                __global const int *src1,
                                __global int *dst) {
    int gid = get_global_id(0);
    dst[gid] = src0[gid] + src1[gid];
}