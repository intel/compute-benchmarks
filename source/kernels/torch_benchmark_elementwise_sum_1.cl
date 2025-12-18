/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

typedef struct {
    float v1;
    __global double *p2;
    __global int *p3;
    size_t size2_;
    size_t size3_;
} CopyableObject;

#define KERNEL_SUM_1(DATA_TYPE) \
__kernel void elementwise_sum_1_##DATA_TYPE(__global const DATA_TYPE *src, \
                                            __global DATA_TYPE *dst) { \
    int gid = get_global_id(0); \
    dst[gid] = src[gid]; \
}

KERNEL_SUM_1(double)
KERNEL_SUM_1(float)
KERNEL_SUM_1(int)

__kernel void elementwise_sum_1_copyable_obj(__global const CopyableObject *src,
                                         __global CopyableObject *dst) {
    int gid = get_global_id(0);
    dst[gid].v1 = src[gid].v1;
    dst[gid].p2 = src[gid].p2;
    dst[gid].p3 = src[gid].p3;
    dst[gid].size2_ = src[gid].size2_;
    dst[gid].size3_ = src[gid].size3_;
}
