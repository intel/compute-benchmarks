/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_one(__global int *outBuffer) {
    outBuffer[get_global_id(0)] = 1;
}

kernel void write_one_with_args(__global uint* outBuffer, __local uint* arg2, uint arg3) {
    outBuffer[get_global_id(0)] = 1;
}