/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_one(__global int *outBuffer) {
    int value = outBuffer[get_global_id(0)];
    if(value == 1)
        outBuffer[get_global_id(0)] = 1;
}

__kernel void only_write_one(__global int *outBuffer) {
    outBuffer[get_global_id(0)] = 1;
}