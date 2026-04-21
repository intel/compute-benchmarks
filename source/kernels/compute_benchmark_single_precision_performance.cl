/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#define MAD_OP_8(a, mul, add) \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);     \
    a = mad(a, mul, add);

#define MAD_OP_128(a, mul, add) \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add)       \
    MAD_OP_8(a, mul, add);

__kernel void single_precision_performance(__global float *buffer, const float multiplier, const float addend, const uint iterations) {
    __global float *address = buffer + get_global_id(0);
    float a = *address;

    for (uint i = 0; i < iterations; i++) {
        MAD_OP_128(a, multiplier, addend);
    }

    *address = a;
}
