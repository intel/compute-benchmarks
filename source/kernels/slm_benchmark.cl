/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void eat_time(int operationsCount, __global int *outBuffer, __local int *slm) {
    volatile int value = 1u;

    for(int i = 0; i < operationsCount; i++){
        value /=2;
        value *=2;
    }

    if(operationsCount < 0 || get_global_id(0) == 0){
        int sum = 0;
        for(int i = 0; i < get_local_size(0); i++){
            sum += slm[i];
        }
        slm[get_local_id(0)] = sum;
        outBuffer[get_global_id(0)] = slm[get_local_id(0)];
    }
}