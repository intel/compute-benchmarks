/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void copy_buffer(__global int *src, __global int *dst) {
    const uint gid = get_global_id(0);
    dst[gid] = src[gid];
}
