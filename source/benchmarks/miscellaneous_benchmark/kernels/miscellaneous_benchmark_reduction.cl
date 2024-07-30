/*
 * Copyright (C) 2022-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void fill_with_ones(__global int *buffer) {
    const uint gid = get_global_id(0);
    buffer[gid] = 1;
}
