/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void write_multiple(__global int *outBuffer) {
    const uint local_id = get_local_id(0);
    const uint local_size = get_local_size(0);

    const uint group_count = get_num_groups(0);

    for(uint i=0;i<group_count;++i){
        const uint x = local_id + i*local_size;
        outBuffer[x] += 1;
    }
    
    //
}

