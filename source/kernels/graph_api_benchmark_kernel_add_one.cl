/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void add_one(__global uint *buffer) {
    size_t id = get_global_id(0);
    buffer[id] += 1;
}
