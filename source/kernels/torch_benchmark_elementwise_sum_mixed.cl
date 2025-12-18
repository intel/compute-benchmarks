/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void elementwise_sum_mixed(__global const double *src1_0,
                                    __global const double *src1_1,
                                    __global const double *src1_2,
                                    __global const float *src2_0,
                                    __global const float *src2_1,
                                    __global const float *src2_2,
                                    __global const float *src2_3,
                                    __global const int *src3_0,
                                    __global const int *src3_1,
                                    __global const int *src3_2,
                                    __global double *dst) {
    int gid = get_global_id(0);
    dst[gid] = src1_0[gid] + src1_1[gid] + src1_2[gid]
             + src2_0[gid] + src2_1[gid] + src2_2[gid] + src2_3[gid]
             + src3_0[gid] + src3_1[gid] + src3_2[gid];
}
