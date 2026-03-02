/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void add_element_constant(__global float *dest,
                                    __global const float *src0,
                                    const float src1) {
    int gid = get_global_id(0);
    dest[gid] = src0[gid] + src1;
}
