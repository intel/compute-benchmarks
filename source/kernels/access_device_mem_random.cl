/*
 * Copyright (C) 2023-2025 Intel Corporation
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

kernel void access_multiple_resources_random(__global uint *fst_src_buffer,
                                            __global uint *snd_src_buffer,
                                            __global uint *offset_buffer,
                                            __global char *result,
                                            uint fst_max_access_index,
                                            uint snd_max_access_index) {
    const uint gid = get_global_id(0);
    const uint impossible_value = 0xFEEDBEAD;
    uint offset = offset_buffer[gid];

    // Read and write to first resource
    uint fst_off = offset % fst_max_access_index;
    if (impossible_value == fst_src_buffer[fst_off]) {
        *result = fst_src_buffer[fst_off];
    }
    fst_src_buffer[fst_off] = 0xFFAABBCC;

    // Read and write to second resource
    uint snd_off = offset % snd_max_access_index;
    if (impossible_value == snd_src_buffer[snd_off]) {
        *result = snd_src_buffer[snd_off];
    }
    snd_src_buffer[snd_off] = 0xFFAABBCC;
}
