/*
 * Copyright (C) 2023-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

 __constant unsigned long globalConsts[8] = {0,1,2,3,4,5,6,7};

__kernel void write_one(__global int *outBuffer) {
    
    outBuffer[get_global_id(0)] = 1 + globalConsts[get_global_id(0)%8];
}

