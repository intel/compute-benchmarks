/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void torch_benchmark_add_element_constant(__global const float *src0,
                                                    const float src1,
                                                    __global float *dst) {
    int gid = get_global_id(0);
    dst[gid] = src0[gid] + src1;
}
