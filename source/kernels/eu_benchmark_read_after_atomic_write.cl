/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

unsigned int shuffleIds(unsigned int id,
                        __local unsigned int *localMemCounter,
                        __local unsigned int *localMem) {
    if (get_local_id(0) == 0) {
        localMemCounter = 0;
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    localMem[atomic_inc(localMemCounter)] = id;
    barrier(CLK_LOCAL_MEM_FENCE);
    return atomic_dec(localMemCounter);
}

__kernel void read_after_atomic_write(__global unsigned int *buffer,
                                      uint useAtomic,
                                      uint shuffleRead,
                                      uint iterations) {
    const unsigned int id = get_global_id(0);
    __local unsigned int localMemCounter;
    __local unsigned int localMem[512];

    unsigned int value = 3;
    for (int i = 0; i < iterations; i++) {
        // Prepare IDs
        unsigned int idForRead = id;
        if (shuffleRead) {
            idForRead = shuffleIds(idForRead, &localMemCounter, localMem);
        }
        const unsigned int idForWrite = id;

        // Perform write
        if (useAtomic) {
            atomic_add(&buffer[idForWrite], value);
        } else {
            buffer[idForWrite] += value;
        }

        // Perform read
        value = buffer[idForRead];

        // Synchronize
        barrier(CLK_LOCAL_MEM_FENCE);
    }
}
