/*
 * Copyright (C) 2025-2026 Intel Corporation
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
__kernel void elementwise_sum_1_##DATA_TYPE(__global DATA_TYPE *dest, \
                                            __global const DATA_TYPE *src) { \
    int gid = get_global_id(0); \
    dest[gid] = src[gid]; \
}

KERNEL_SUM_1(double)
KERNEL_SUM_1(float)
KERNEL_SUM_1(int)

__kernel void elementwise_sum_1_copyable_obj(__global CopyableObject *dest,
                                         __global const CopyableObject *src) {
    int gid = get_global_id(0);
    dest[gid].v1 = src[gid].v1;
    dest[gid].p2 = src[gid].p2;
    dest[gid].p3 = src[gid].p3;
    dest[gid].size2_ = src[gid].size2_;
    dest[gid].size3_ = src[gid].size3_;
}
