/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#if defined(cl_khr_fp16)
#pragma OPENCL EXTENSION cl_khr_fp16 : enable
#endif
#if defined(cl_khr_fp64)
#pragma OPENCL EXTENSION cl_khr_fp64 : enable
#endif

#if defined(INTEGER_TYPE)
#define MAD(value, mul, add) ((value) * (mul) + (add))
#else
#define MAD(value, mul, add) mad((value), (mul), (add))
#endif

__kernel void mad_performance(__global VECTOR_TYPE *buffer, const SCALAR_TYPE multiplier, const SCALAR_TYPE addend, const uint iterations) {
    __global VECTOR_TYPE *address = buffer + get_global_id(0);
    const VECTOR_TYPE mul = (VECTOR_TYPE)(multiplier);
    const VECTOR_TYPE add = (VECTOR_TYPE)(addend);
    VECTOR_TYPE value = *address;

    for (uint i = 0; i < iterations; i++) {
#pragma unroll
        for (uint j = 0; j < MADS_PER_LOOP; j++) {
            value = MAD(value, mul, add);
        }
    }

    *address = value;
}
