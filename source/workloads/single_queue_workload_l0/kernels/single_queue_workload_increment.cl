/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void increment(__global int *buffer, int operationsCount) {
    size_t id = get_global_id(0);
    int value = buffer[id];

    for (int i = 0; i < operationsCount; i++) {
        value /= 2;
        value *= 2;
    }
    value += 1;

    buffer[id] = value;
}
