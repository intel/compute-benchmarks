/*
 * Copyright (C) 2023-2024 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

kernel void access_device_memory_random(__global uint *src_buffer,
                                        __global uint *offset_buffer,
                                        __global char *result,
                                        uint access_mode,
                                        uint max_access_index) {
    const uint gid = get_global_id(0);
    const uint impossible_value = 0xFEEDBEAD;
    uint off = offset_buffer[gid];
    off = off % max_access_index;

    // Read or Read-Write
    if (access_mode == 0 || access_mode == 2) {
        // Avoid the read being optimized out
        if (impossible_value == src_buffer[off]) {
            *result = src_buffer[off];
        }
    }

    // Write or Read-Write
    if (access_mode == 1 || access_mode == 2) {
        src_buffer[off] = 0xFFAABBCC;
    }
}
