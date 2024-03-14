/*
 * Copyright (C) 2022-2024 Intel Corporation
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

__kernel void write_random(__global STREAM_TYPE *restrict x) {
    const char data[] = {0xFF, 0xEE, 0xFE, 0x01, 0xE0, 0xD0, 0xF2, 0x0F, 0x1C,
                         0xF1, 0xEF, 0xE3, 0xD3, 0xA1, 0xF0, 0x3E, 0xE1, 0x2E,
                         0xED, 0xF0, 0x99, 0xE2, 0x0E, 0x0E, 0x20, 0xEF, 0x0D,
                         0xD4, 0xEF, 0xCB, 0x1F, 0xF2, 0x33, 0xFF, 0xFE, 0x01,
                         0x13, 0xF5, 0x00, 0x2E, 0x0E, 0x22, 0x32, 0x2F, 0x00,
                         0x10, 0xFD, 0xF1, 0x0D, 0xB1, 0x01, 0x0F, 0xDD, 0x4E,
                         0x0F, 0xDD, 0x4A, 0xE1, 0x2E, 0xD5, 0x1E, 0xDE, 0x23,
                         0xE0, 0x30, 0xA0, 0x23, 0x22, 0x0F, 0x2C, 0x0E, 0x03,
                         0x9E, 0x2D, 0xC0, 0x01, 0x04, 0x2F, 0x02, 0x36, 0x1F,
                         0xE6, 0xE1, 0xFA, 0xF2, 0x10, 0xCD, 0xD5, 0x2E, 0x52,
                         0x0F, 0x00, 0x13, 0x34, 0x2E, 0xEF, 0xCF, 0x01, 0x1E,
                         0x3C, 0x00, 0x22, 0x4C, 0x2B, 0xF9, 0x10, 0x4D, 0x0D,
                         0x1E, 0x2F, 0x21, 0xC2, 0x13, 0xF1, 0x10, 0xAD, 0x01,
                         0xFF, 0x02, 0x6E, 0xD3, 0xE4, 0xDC, 0x00, 0x0C, 0x03,
                         0x21, 0xF2, 0x53, 0x1E, 0x2C, 0xC2, 0x11, 0xBF, 0xFF,
                         0xC5, 0xF0, 0xEF, 0x12, 0x7F, 0x00, 0x2D, 0x02, 0xF2,
                         0x3D, 0xF4, 0xFE, 0x11, 0xC3, 0xD1, 0x00, 0x10, 0xEE,
                         0xFE, 0xEC, 0xA0, 0xFB, 0x1F, 0xD0, 0xFE, 0x33, 0x4F,
                         0xFE, 0x20, 0x0F, 0x22, 0x22, 0xFD, 0x13, 0x26, 0x0D,
                         0xF0, 0x52, 0x32, 0xF5, 0xDE, 0x02, 0x2E, 0xD1, 0x50,
                         0xFD, 0x22, 0x30, 0x41, 0xE5, 0x4D, 0xE0, 0x70, 0x1F,
                         0xCF, 0xB0, 0x20, 0x01, 0xDF, 0xB1, 0x00, 0x22, 0xF1,
                         0xFF, 0xF2, 0xE1, 0xE3, 0x03, 0xFD, 0xF3, 0xDA, 0xE1,
                         0x2D, 0xF0, 0xFF, 0xF3, 0xED, 0x0D, 0xEC, 0x0E, 0xEC,
                         0xFF, 0x0F, 0x04, 0xFD, 0x01, 0x20, 0x41, 0xEC, 0x2D,
                         0xEF, 0x3D, 0xF0, 0xF0, 0x75, 0xCF, 0xD1, 0x11, 0x01,
                         0xE0, 0x01, 0xDD, 0x2F, 0xF1, 0xF1, 0x2E, 0xDF, 0x32,
                         0xF3, 0x0B, 0x21, 0xCB, 0x32, 0xE3, 0x1F, 0xFF, 0x12,
                         0xF2, 0xE3, 0xC0, 0xB0};
    const int dataSize = 256;
    const int i = get_global_id(0);
    const int dataIndex = i % (dataSize / sizeof(STREAM_TYPE));
    const STREAM_TYPE dataToWrite = ((STREAM_TYPE *)(data))[dataIndex];
    x[i] = dataToWrite;
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
                           __global STREAM_TYPE *restrict z, uint workItemGroupSize, const int remoteAccessFraction) {
    int g_id = get_global_id(0);
    if (remoteAccessFraction != 0) {
        const size_t gws = get_global_size(0);
        const int cache_line_id = g_id / workItemGroupSize;
        if (cache_line_id % remoteAccessFraction == 0) {
            g_id = (g_id + gws / 2) % gws;
        }
    }

    z[g_id] = x[g_id] + y[g_id];
}

__kernel void remote_read(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, uint workItemGroupSize, const int remoteAccessFraction) {
    int g_id = get_global_id(0);
    if (remoteAccessFraction != 0) {
        const size_t gws = get_global_size(0);
        const int cache_line_id = g_id / workItemGroupSize;
        if (cache_line_id % remoteAccessFraction == 0) {
            g_id = (g_id + gws / 2) % gws;
        }
    }

    STREAM_TYPE value = x[g_id];
    if (value == 37) {
        *dummyOutput = value;
    }
}

#ifdef ELEMENT_SIZE
__kernel void full_remote_block_read(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    const uint sgid = get_sub_group_local_id();
    const size_t sgs = get_sub_group_size();
    // pointer for intel_sub_group_block_read must be the same for all work items in the sub-group
    const uint startIndex = ((gid - sgid + gws / 2) & (gws-1)) * iterations;

    for (uint i = startIndex; i < startIndex + iterations * sgs; i += sgs) {
        #if ELEMENT_SIZE < 4
            STREAM_TYPE value = x[i + sgid];
        #elif ELEMENT_SIZE == 4
            uint value = intel_sub_group_block_read(x + i);
        #elif ELEMENT_SIZE == 8
            uint2 value = intel_sub_group_block_read2(x + i);
        #endif
        
        #if ELEMENT_SIZE < 8
            if (value == 33) {
                *dummyOutput = value;
            }
        #else
            if (value.x == 33) {
                *dummyOutput = value.y;
            }
        #endif
    }
}
#endif

#ifdef USED_SLM
__kernel void full_remote_block_read_xe_cores_distributed(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    __local char slm[USED_SLM];
    const uint lid = get_local_id(0);
    const uint gid = get_global_id(0);
    const uint sgid = get_sub_group_local_id();
    const size_t lws = get_local_size(0);
    const size_t gws = get_global_size(0);
    const size_t sgs = get_sub_group_size();
    const uint startIndex = ((gid - sgid + gws / 2) & (gws-1)) * iterations;

    // Ensure that compiler won't optimize away the SLM
    if (lid > lws) { 
        for (uint i = 0; i < USED_SLM; i++) {
            slm[i] = 0;
        }
    }
    
    for (uint i = startIndex; i < startIndex + iterations * sgs; i += sgs) {
        #if ELEMENT_SIZE < 4
            STREAM_TYPE value = x[i + sgid];
        #elif ELEMENT_SIZE == 4
            uint value = intel_sub_group_block_read(x + i);
        #elif ELEMENT_SIZE == 8
            uint2 value = intel_sub_group_block_read2(x + i);
        #endif

        // Ensure that compiler won't optimize away the read
        #if ELEMENT_SIZE < 8
            if (value == 33) {
                slm[i % (USED_SLM)] = value;
            }
        #else
            if (value.x == 33) {
                slm[i % (USED_SLM)] = value.y;
            }
        #endif

    }

    if (lid > lws) {
        dummyOutput[gid] = slm[lid];
    }
}
#endif

__kernel void full_remote_scatter_read(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    // First half of workitems access memory starting from middle of the buffer
    // Second half access memory starting from 0 (to access memory only from a remote tile)
    const uint startIndex = (gid & (gws/2-1)) + (gid < gws/2) * (bufferLength / 2);
    // To assure there will not be a cache hit we have to access memory with a minimum 64B gap between
    // We also want to access all of the buffer, so if there will be more than 128 workitems 
    // e.g. 256 they have to read with 128B interval to prevent reading the same memory twice
    const uint cachelineGap = 64 / sizeof(STREAM_TYPE) * (1 + gws / 128);

    for (uint i = 0; i < iterations; i++) {
        // Fold up calculated offset to prevent exceeding buffer length
        STREAM_TYPE value = x[startIndex + ((i * cachelineGap) & (bufferLength / 2 - 1))];
        if (value == 33) {
            *dummyOutput = value;
        }
    }
}

#ifdef USED_SLM
__kernel void full_remote_scatter_read_xe_cores_distributed(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    __local char slm[USED_SLM];
    const uint lid = get_local_id(0);
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    const size_t lws = get_local_size(0);
    const uint startIndex = (gid & (gws/2-1)) + (gid < gws/2) * (bufferLength / 2);
    const uint cachelineGap = 64 / sizeof(STREAM_TYPE) * gws / 128;

    // Ensure that compiler won't optimize away the SLM
    if (lid > lws) { 
        for (uint i = startIndex; i < USED_SLM; i++) {
            slm[i] = 0;
        }
    }
    
    for (uint i = 0; i < iterations; i++) {
        STREAM_TYPE value = x[startIndex + ((i * cachelineGap) & (bufferLength / 2 - 1))];

        // Ensure that compiler won't optimize away the read
        if (value == 33) {
            slm[i % (USED_SLM)] = value;
        }
    }

    if (lid > lws) {
        dummyOutput[gid] = slm[lid];
    }
}
#endif

__kernel void remote_write(__global STREAM_TYPE *restrict x, uint workItemGroupSize, const int remoteAccessFraction) {
    int g_id = get_global_id(0);
    if (remoteAccessFraction != 0) {
        const size_t gws = get_global_size(0);
        const int cache_line_id = g_id / workItemGroupSize;
        if (cache_line_id % remoteAccessFraction == 0) {
            g_id = (g_id + gws / 2) % gws;
        }
    }

    x[g_id] = 37;
}

#ifdef ELEMENT_SIZE
__kernel void full_remote_block_write(__global STREAM_TYPE *restrict x, const uint bufferLength, const uint iterations) {
    const uint gid = get_global_id(0);
    const uint sgid = get_sub_group_local_id();
    const size_t gws = get_global_size(0);
    const size_t sgs = get_sub_group_size();
    // pointer for intel_sub_group_block_write must be the same for all work items in the sub-group
    const uint startIndex = ((gid - sgid + gws / 2) & (gws-1)) * iterations;

    for (uint i = startIndex; i < startIndex + iterations * sgs; i += sgs) {
        #if ELEMENT_SIZE < 4
            x[i+sgid] = 37;
        #elif ELEMENT_SIZE == 4
            intel_sub_group_block_write(x + i, 37);
        #elif ELEMENT_SIZE == 8
            intel_sub_group_block_write2(x + i, 37);
        #endif
    }
}
#endif

#ifdef USED_SLM
__kernel void full_remote_block_write_xe_cores_distributed(__global STREAM_TYPE *restrict x, const uint bufferLength, const uint iterations) {
    __local char slm[USED_SLM];
    const uint lid = get_local_id(0);
    const uint gid = get_global_id(0);
    const uint sgid = get_sub_group_local_id();
    const size_t lws = get_local_size(0);
    const size_t gws = get_global_size(0);
    const size_t sgs = get_sub_group_size();
    const uint startIndex = ((gid - sgid + gws / 2) & (gws-1)) * iterations;

    // Ensure that compiler won't optimize away the SLM
    if (lid > lws) { 
        for (uint i = startIndex; i < USED_SLM; i++) {
            slm[i] = 0;
        }
    }
    
    for (uint i = startIndex; i < startIndex + iterations * sgs; i += sgs) {
        #if ELEMENT_SIZE < 4
            x[i+sgid] = 37;
        #elif ELEMENT_SIZE == 4
            intel_sub_group_block_write(x + i, 37);
        #elif ELEMENT_SIZE == 8
            intel_sub_group_block_write2(x + i, 37);
        #endif
    }

    if (lid > lws) {
        x[gid] = slm[lid];
    }
}
#endif

__kernel void full_remote_scatter_write(__global STREAM_TYPE *restrict x, const uint bufferLength, const uint iterations) {
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    const uint startIndex = (gid & (gws/2-1)) + (gid < gws/2) * (bufferLength / 2);
    const uint cachelineGap = 64 / sizeof(STREAM_TYPE) * gws / 128;

    for (uint i = 0; i < iterations; i++) {
        x[startIndex + ((i * cachelineGap) & (bufferLength / 2 - 1))] = 37;
    }
}

#ifdef USED_SLM
__kernel void full_remote_scatter_write_xe_cores_distributed(__global STREAM_TYPE *restrict x, const uint bufferLength, const uint iterations) {
    __local char slm[USED_SLM];
    const uint lid = get_local_id(0);
    const uint gid = get_global_id(0);
    const size_t lws = get_local_size(0);
    const size_t gws = get_global_size(0);
    const uint startIndex = (gid & (gws/2-1)) + (gid < gws/2) * (bufferLength / 2);
    const uint cachelineGap = 64 / sizeof(STREAM_TYPE) * gws / 128;

    // Ensure that compiler won't optimize away the SLM
    if (lid > lws) { 
        for (uint i = startIndex; i < USED_SLM; i++) {
            slm[i] = 0;
        }
    }
    
    for (uint i = 0; i < iterations; i++) {
        x[startIndex + ((i * cachelineGap) & (bufferLength / 2 - 1))] = 37;
    }

    if (lid > lws) {
        x[gid] = slm[lid];
    }
}
#endif

__kernel void remote_max_saturation(__global STREAM_TYPE *restrict x, uint workItemGroupSize, const int remoteAccessFraction, const int writesPerWorkgroup) {
    if (get_local_id(0) < writesPerWorkgroup) {
        int g_id = get_global_id(0);
        if (remoteAccessFraction != 0) {
            const size_t gws = get_global_size(0);
            const int cache_line_id = g_id / workItemGroupSize;
            if (cache_line_id % remoteAccessFraction == 0) {
                g_id = (g_id + gws / 2) % gws;
            }
        }

        x[g_id] = 37;
    }
}

__kernel void stream_3bytesRGBtoY(const __global uchar *restrict x, __global uchar *restrict luminance, float scalar) {
    const int i = get_global_id(0) * 3;
    const int y = get_global_id(0);
    luminance[y] = (uchar)((x[i] * 0.2126f) + (x[i + 1] * 0.7152f) + (x[i + 2] * 0.0722f));
}

__kernel void stream_3BytesAlignedRGBtoY(const __global uchar4 *restrict x, __global uchar *restrict luminance, float scalar) {
    const int i = get_global_id(0);
    luminance[i] = (uchar)((x[i].x * 0.2126f) + (x[i].y * 0.7152f) + (x[i].z * 0.0722f));
}
