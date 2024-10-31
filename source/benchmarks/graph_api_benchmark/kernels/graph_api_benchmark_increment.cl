/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

/* __kernel void increment(__global int* array, __global int size) {
    int idx = get_global_id(0);
    if (idx < size)
        array[idx] += 1;
} */
__kernel void increment(__global int* array) {
    int idx = get_global_id(0);
    array[idx] += 1;
}