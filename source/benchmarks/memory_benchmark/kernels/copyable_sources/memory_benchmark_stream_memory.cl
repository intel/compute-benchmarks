/*
 * Copyright (C) 2022 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// #pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void read(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, STREAM_TYPE scalar) {
    const int i = get_global_id(0);
    STREAM_TYPE value = x[i];

    // A trick to ensure compiler won't optimize away the read
    if (value == 0.37221) {
        *dummyOutput = value;
    }
}

__kernel void write(__global STREAM_TYPE *restrict x, STREAM_TYPE scalar) {
    const int i = get_global_id(0);
    x[i] = scalar;
}

__kernel void scale(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict y, STREAM_TYPE scalar) {
    const int i = get_global_id(0);
    y[i] = x[i] * scalar;
}

__kernel void triad(const __global STREAM_TYPE *restrict x, const __global STREAM_TYPE *restrict y,
                    __global STREAM_TYPE *restrict z, STREAM_TYPE scalar) {
    const int i = get_global_id(0);
    z[i] = x[i] + y[i] * scalar;
}

__kernel void remote_triad(const __global STREAM_TYPE *restrict x, const __global STREAM_TYPE *restrict y,
                    __global STREAM_TYPE *restrict z, STREAM_TYPE scalar, const int remoteAccessFraction) {
    const int j = get_global_id(0);
    const int cache_line_id = j /16;
    const size_t N = get_global_size(0);
    int i;
    if(remoteAccessFraction != 0 && cache_line_id % remoteAccessFraction == 0){
        i = (j+N/2)%N;
    } else {
        i = j;
    }
    z[i] = x[i] + y[i];
}

__kernel void stream_3bytesRGBtoY(const __global uchar *restrict x, __global uchar *restrict luminance, float scalar) {
    const int i = get_global_id(0) * 3;
    const int y = get_global_id(0);
    luminance[y] = (uchar)( (x[i]*0.2126f) + (x[i+1]*0.7152f) + (x[i+2]*0.0722f) );
}

__kernel void stream_3BytesAlignedRGBtoY(const __global uchar4 *restrict x, __global uchar *restrict luminance, float scalar) {
    const int i = get_global_id(0);
    luminance[i] = (uchar)( (x[i].x *0.2126f) + (x[i].y * 0.7152f) + (x[i].z * 0.0722f) );
}


