/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_int(__global int *ptr1, const int offset1, __global int *ptr2, const int offset2) {
    int id = get_global_id(0);
    if (id == 0) {
        ptr1[offset1] = offset1;
        ptr2[offset2] = offset2;
    }
}

__kernel void write_float(__global float *ptr1, const int offset1, __global float *ptr2, const int offset2) {
    int id = get_global_id(0);
    if (id == 0) {
        ptr1[offset1] = (float)offset1;
        ptr2[offset2] = (float)offset2;
    }
}
