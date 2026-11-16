/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void simple_add(int elementCount, global const int *input, global int *output) {
    for (int i = 0; i < elementCount; ++i) {
        output[i] = input[i] + 1;
    }
}
