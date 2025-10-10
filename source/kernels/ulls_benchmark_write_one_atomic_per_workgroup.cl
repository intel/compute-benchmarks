/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_one(__global int *outBuffer) {
    if(get_local_id(0) == 0){
        int value = atomic_dec(outBuffer);
        if(value == 1 ) {
            outBuffer[1] = 1337;
        }
    }
}