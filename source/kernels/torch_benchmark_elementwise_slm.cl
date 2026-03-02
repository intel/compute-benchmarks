/*
 * Copyright (C) 2025-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void elementwise_slm(__global float *dest, int slm_num, __local float* slm) {
    // Allocate local memory for the work-group
    // size in bytes declared at kernel launch by last "hidden" argument
	
    int local_id = get_local_id(0);
    int local_size = get_local_size(0);
		
    // Fill local memory
    for (int i = local_id; i < slm_num; i += local_size) {
        slm[i] = 0.1f * i;
    }
    
    barrier(CLK_LOCAL_MEM_FENCE);
    
    // Write result from the last element in SLM and checker value
    if (local_id == 0) {
        dest[0] = slm[slm_num - 1];
        dest[1] = 13.0f;
    }
}
