/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void kernel_assign(__global float * dest, __global float * source) {
    int id = get_global_id(0);
    dest[id] = source[id];
}
