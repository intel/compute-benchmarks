/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void compute_bound(__global const float *src, __global float *dst, uint iterations) {
    size_t id = get_global_id(0);
    float a = src[id];
    float b = a + 1.0f;
    float c = a + 2.0f;
    float d = a + 3.0f;
    for (uint i = 0; i < iterations; i++) {
        a = fma(a, 0.999f, 0.001f);
        b = fma(b, 0.998f, 0.002f);
        c = fma(c, 0.997f, 0.003f);
        d = fma(d, 0.996f, 0.004f);
    }
    dst[id] = a + b + c + d;
}
