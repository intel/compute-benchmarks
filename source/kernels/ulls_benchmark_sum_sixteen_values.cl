/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void sumSixteenValues(const global float *input, global float *output) {
    const size_t localSize = get_local_size(0);
    size_t index = get_group_id(0) * localSize * 16 + get_local_id(0);
    float sum = 0.0f;
    for (int i = 0; i < 16; i++) {
        sum += input[index];
        index += localSize;
    }
    output[get_global_id(0)] = sum;
}
