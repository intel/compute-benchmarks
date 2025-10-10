/*
 * Copyright (C) 2024-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void dummy_compute(global char* x, const int iters)
{
    const ulong idx = get_global_id(0);
    for (int i = 0; i < iters; i++) {
        x[idx] += x[idx] / 3 + 2;
    }
}
