/*
 * Copyright (C) 2022-2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

// #pragma OPENCL EXTENSION cl_khr_fp64 : enable

__kernel void readWithMultiplier(const __global volatile STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, STREAM_TYPE scalar, int multiplier) {
    int i = get_global_id(0);
    if (multiplier > 1) {
        i = i * multiplier;
        if (i >= get_global_size(0))
            return;
    }
    STREAM_TYPE value = x[i];
}

__kernel void read(const __global volatile STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, STREAM_TYPE scalar) {
    const int i = get_global_id(0);
    STREAM_TYPE value = x[i];
}

__kernel void writeWithMultiplier(__global STREAM_TYPE *restrict x, STREAM_TYPE scalar, int multiplier) {
    int i = get_global_id(0);
    if (multiplier > 1) {
        i = i * multiplier;
        if (i >= get_global_size(0))
            return;
    }
    x[i] = scalar;
}

__kernel void write(__global STREAM_TYPE *restrict x, STREAM_TYPE scalar) {
    const int i = get_global_id(0);
    x[i] = scalar;
}

#ifdef WRITE_RANDOM_PATTERN
__kernel void write_random(__global STREAM_TYPE *restrict x) {
    const char data[] = {WRITE_RANDOM_PATTERN};
    const int dataSize = 256;
    const int i = get_global_id(0);
    const int dataIndex = i % (dataSize / sizeof(STREAM_TYPE));
    const STREAM_TYPE dataToWrite = ((STREAM_TYPE *)(data))[dataIndex];
    x[i] = dataToWrite;
}
#endif

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

__kernel void remote_read(const __global volatile STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, uint workItemGroupSize, const int remoteAccessFraction) {
    int g_id = get_global_id(0);
    if (remoteAccessFraction != 0) {
        const size_t gws = get_global_size(0);
        const int cache_line_id = g_id / workItemGroupSize;
        if (cache_line_id % remoteAccessFraction == 0) {
            g_id = (g_id + gws / 2) % gws;
        }
    }

    STREAM_TYPE value = x[g_id];
}

#ifdef ELEMENT_SIZE
__kernel void full_remote_block_read(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    const uint sgid = get_sub_group_local_id();
    const size_t sgs = get_sub_group_size();
    // pointer for intel_sub_group_block_read must be the same for all work items in the sub-group
    const uint startIndex = ((gid - sgid + gws / 2) & (gws - 1)) * iterations;

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
    const uint startIndex = ((gid - sgid + gws / 2) & (gws - 1)) * iterations;

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

__kernel void full_remote_scatter_read(const __global volatile STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    // First half of workitems access memory starting from middle of the buffer
    // Second half access memory starting from 0 (to access memory only from a remote tile)
    const uint startIndex = (gid & (gws / 2 - 1)) + (gid < gws / 2) * (bufferLength / 2);
    // To assure there will not be a cache hit we have to access memory with a minimum 64B gap between
    // We also want to access all of the buffer, so if there will be more than 128 workitems
    // e.g. 256 they have to read with 128B interval to prevent reading the same memory twice
    const uint cachelineGap = 64 / sizeof(STREAM_TYPE) * (1 + gws / 128);

    for (uint i = 0; i < iterations; i++) {
        // Fold up calculated offset to prevent exceeding buffer length
        STREAM_TYPE value = x[startIndex + ((i * cachelineGap) & (bufferLength / 2 - 1))];
    }
}

#ifdef USED_SLM
__kernel void full_remote_scatter_read_xe_cores_distributed(const __global STREAM_TYPE *restrict x, __global STREAM_TYPE *restrict dummyOutput, const uint bufferLength, const uint iterations) {
    __local char slm[USED_SLM];
    const uint lid = get_local_id(0);
    const uint gid = get_global_id(0);
    const size_t gws = get_global_size(0);
    const size_t lws = get_local_size(0);
    const uint startIndex = (gid & (gws / 2 - 1)) + (gid < gws / 2) * (bufferLength / 2);
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
    const uint startIndex = ((gid - sgid + gws / 2) & (gws - 1)) * iterations;

    for (uint i = startIndex; i < startIndex + iterations * sgs; i += sgs) {
#if ELEMENT_SIZE < 4
        x[i + sgid] = 37;
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
    const uint startIndex = ((gid - sgid + gws / 2) & (gws - 1)) * iterations;

    // Ensure that compiler won't optimize away the SLM
    if (lid > lws) {
        for (uint i = startIndex; i < USED_SLM; i++) {
            slm[i] = 0;
        }
    }

    for (uint i = startIndex; i < startIndex + iterations * sgs; i += sgs) {
#if ELEMENT_SIZE < 4
        x[i + sgid] = 37;
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
    const uint startIndex = (gid & (gws / 2 - 1)) + (gid < gws / 2) * (bufferLength / 2);
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
    const uint startIndex = (gid & (gws / 2 - 1)) + (gid < gws / 2) * (bufferLength / 2);
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
