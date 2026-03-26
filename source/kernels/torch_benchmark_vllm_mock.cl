/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

__kernel void mock_triton_red_fused__to_copy_add_embedding_mean_mul_pow_rsqrt_0(
        __global const int *in_ptr0,
        __global const float *in_ptr1,
        __global const float *in_ptr2,
        __global float *out_ptr0,
        __global float *out_ptr2,
        int xnumel,
        int r0_numel,
        int XBLOCK,
        int R0_BLOCK)
{
    int gid = get_global_id(0);
    gid = gid % r0_numel; // to avoid out of bound access in this mock kernel
    float v = 0.0f;
    if (in_ptr0)
        v += (float)(in_ptr0[0]);
    if (in_ptr1)
        v += in_ptr1[gid];
    if (in_ptr2)
        v += in_ptr2[gid];

    if (out_ptr2)
        out_ptr2[gid] = v;

    if (out_ptr0) {
        int use_params = xnumel + r0_numel + XBLOCK + R0_BLOCK;
        out_ptr0[gid] = v + (float)(use_params);
    }
}

__kernel void mock_reshape_and_cache(
    __global const float *in_ptr0,
    __global const float *in_ptr1,
    __global float *out_ptr0,
    __global float *out_ptr1,
    __global const long *in_ptr2,
    __global const float *in_ptr3,
    __global const float *in_ptr4)
{
    int gid = get_global_id(0);
    gid = gid % 64; // to avoid out of bound access in this mock kernel
    float v = 0;
    if (in_ptr0)
        v += in_ptr0[gid];
    if (in_ptr1)
        v += in_ptr1[gid];
    if (in_ptr2)
        v += (float)in_ptr2[0];
    if (in_ptr3)
        v += (float)in_ptr3[0];
    if (in_ptr4)
        v += (float)in_ptr4[0];

    if (out_ptr0)
        out_ptr0[gid] = v;
    if (out_ptr1)
        out_ptr1[gid] = v;
}

__kernel void mock_flash_attn(
    __global const float *in_ptr0,
    __global const float *in_ptr1,
    __global const float *in_ptr2,
    __global float *out_ptr0,
    __global const int *in_ptr3,
    __global const int *in_ptr4,
    __global const int *in_ptr5,
    __global const int *in_ptr6,
    int scalar0,
    int scalar1,
    float scalar2,
    float scalar3,
    int scalar4,
    int scalar5,
    int scalar6,
    int scalar7,
    float scalar8,
    int scalar9)
{
    int gid = get_global_id(0);
    gid = gid % scalar0;
    float v = 0;
    if (in_ptr0)
        v += in_ptr0[gid];
    if (in_ptr1)
        v += in_ptr1[gid];
    if (in_ptr2)
        v += in_ptr2[gid];
    if (in_ptr3)
        v += (float)in_ptr3[0];
    if (in_ptr4)
        v += (float)in_ptr4[0];
    if (in_ptr5)
        v += (float)in_ptr5[0];
    if (in_ptr6)
        v += (float)in_ptr6[0];

    if (out_ptr0) {
        int use_params = scalar0 + scalar1 + (int)scalar2 + (int)scalar3 + scalar4 +
                     scalar5 + scalar6 + scalar7 + (int)scalar8 + scalar9;
        out_ptr0[gid] = v + (float)use_params;
    }
}

__kernel void mock_triton_poi_fused_1_3(
    __global const float *in_ptr0,
    __global const long *in_ptr1,
    __global const float *in_ptr2,
    __global float *out_ptr0,
    __global float *out_ptr1,
    int xnumel_0,
    int xnumel_1,
    int XBLOCK)
{
    int gid = get_global_id(0);
    gid = gid % xnumel_1;
    float v = 0;
    if (in_ptr0)
        v += in_ptr0[gid];
    if (in_ptr1)
        v += (float)in_ptr1[0];
    if (in_ptr2)
        v += in_ptr2[gid];

    if (out_ptr1)
        out_ptr1[gid] = v;

    if (out_ptr0) {
        int use_params = xnumel_0 + xnumel_1 + XBLOCK;
        out_ptr0[gid] = v + (float)use_params;
    }
}

__kernel void mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0(
    __global const float *in_ptr0,
    __global const float *in_ptr1,
    __global const float *in_ptr2,
    __global float *out_ptr0,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK)
{
    int gid = get_global_id(0);
    gid = gid % xnumel;
    float v = 0;
    if (in_ptr0)
        v += in_ptr0[gid];
    if (in_ptr1)
        v += in_ptr1[gid];
    if (in_ptr2)
        v += in_ptr2[gid];

    if (out_ptr0) {
        int use_params = r0_numel + XBLOCK + R0_BLOCK;
        out_ptr0[gid] = v + (float)use_params;
    }
}

__kernel void mock_triton_poi_fused_mul_silu_slice_1(
    __global const float *in_ptr0,
    __global float *out_ptr0,
    int xnumel,
    int XBLOCK)
{
    int gid = get_global_id(0);
    float v = 0;
    if (in_ptr0)
        v += in_ptr0[gid];

    if (out_ptr0) {
        int use_params = xnumel + XBLOCK;
        out_ptr0[gid] = v + (float)use_params;
    }
}

__kernel void mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2(
    __global const float *in_ptr0,
    __global const float *in_ptr1,
    __global const float *in_ptr2,
    __global const float *in_ptr3,
    __global float *out_ptr0,
    __global float *out_ptr1,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK)
{
    int gid = get_global_id(0);
    gid = gid % xnumel;
    float v = 0;
    if (in_ptr0)
        v += in_ptr0[gid];
    if (in_ptr1)
        v += in_ptr1[gid];
    if (in_ptr2)
        v += in_ptr2[gid];
    if (in_ptr3)
        v += in_ptr3[gid];

    if (out_ptr1)
        out_ptr1[gid] = v;

    if (out_ptr0) {
        int use_params = r0_numel + XBLOCK + R0_BLOCK;
        out_ptr0[gid] = v + (float)use_params;
    }
}

__kernel void mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer(
    __global float *in_out_ptr0,
    __global const float *in_ptr1,
    __global const float *in_ptr2,
    __global const float *in_ptr3,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK)
{
    int gid = get_global_id(0);
    gid = gid % xnumel;
    float v = 0;
    if (in_out_ptr0)
        v += in_out_ptr0[gid];
    if (in_ptr1)
        v += in_ptr1[gid];
    if (in_ptr2)
        v += in_ptr2[gid];
    if (in_ptr3)
        v += in_ptr3[gid];

    if (in_out_ptr0) {
        int use_params = r0_numel + XBLOCK + R0_BLOCK;
        in_out_ptr0[gid] = v + (float)use_params;
    }
}
