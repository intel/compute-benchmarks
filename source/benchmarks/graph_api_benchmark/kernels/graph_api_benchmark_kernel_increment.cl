/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void kernel_increment(uint numIncrements, __global int* d_A) {
    for (uint i = 0; i < numIncrements; i++) {
        *d_A = *d_A + 1;
    }
}
