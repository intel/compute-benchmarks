/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#ifndef VLLM_MOCK_KERNELS_H
#define VLLM_MOCK_KERNELS_H

#include <sycl/sycl.hpp>

namespace syclex = sycl::ext::oneapi::experimental;

template <typename data_type>
void mock_triton_red_fused__to_copy_add_embedding_mean_mul_pow_rsqrt_0(
    sycl::nd_item<1> item,
    const int *in_ptr0,
    const data_type *in_ptr1,
    const data_type *in_ptr2,
    data_type *out_ptr0,
    data_type *out_ptr2,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK) {
    size_t id = item.get_global_id(0);
    id = id % r0_numel; // to avoid out of bound access in this mock kernel
    data_type v = 0;
    if (in_ptr0)
        v += static_cast<data_type>(in_ptr0[0]);
    if (in_ptr1)
        v += in_ptr1[id];
    if (in_ptr2)
        v += in_ptr2[id];

    if (out_ptr2)
        out_ptr2[id] = v;

    if (out_ptr0) {
        int use_params = xnumel + r0_numel + XBLOCK + R0_BLOCK;
        out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_fused_embedding_layernorm(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                             bool useEvents,
                                             const int *in_ptr0,
                                             const data_type *in_ptr1,
                                             const data_type *in_ptr2,
                                             data_type *out_ptr0,
                                             data_type *out_ptr2,
                                             int xnumel,
                                             int r0_numel,
                                             int XBLOCK,
                                             int R0_BLOCK) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_embedding_mean_mul_pow_rsqrt_0<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, out_ptr2, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_embedding_mean_mul_pow_rsqrt_0<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, out_ptr2, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    }
}

template <typename data_type>
void mock_triton_poi_fused_1_3(
    sycl::nd_item<1> item,
    const data_type *in_ptr0,
    const long *in_ptr1,
    const data_type *in_ptr2,
    data_type *out_ptr0,
    data_type *out_ptr1,
    int xnumel_0,
    int xnumel_1,
    int XBLOCK) {
    size_t id = item.get_global_id(0);
    id = id % xnumel_1;
    data_type v = 0;
    if (in_ptr0)
        v += in_ptr0[id];
    if (in_ptr1)
        v += static_cast<data_type>(in_ptr1[0]);
    if (in_ptr2)
        v += in_ptr2[id];

    if (out_ptr1)
        out_ptr1[id] = v;

    if (out_ptr0) {
        int use_params = xnumel_0 + xnumel_1 + XBLOCK;
        out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_mock_triton_poi_fused_1_3(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                             bool useEvents,
                                             const data_type *in_ptr0,
                                             const long *in_ptr1,
                                             const data_type *in_ptr2,
                                             data_type *out_ptr0,
                                             data_type *out_ptr1,
                                             int xnumel_0,
                                             int xnumel_1,
                                             int XBLOCK) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_poi_fused_1_3<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, out_ptr1, xnumel_0, xnumel_1, XBLOCK);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_poi_fused_1_3<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, out_ptr1, xnumel_0, xnumel_1, XBLOCK);
        });
    }
}

template <typename data_type>
void mock_reshape_and_cache(
    sycl::nd_item<1> item,
    const data_type *in_ptr0,
    const data_type *in_ptr1,
    data_type *out_ptr0,
    data_type *out_ptr1,
    long *in_ptr2,
    float *in_ptr3,
    float *in_ptr4) {
    size_t id = item.get_global_id(0);
    id = id % 64; // to avoid out of bound access in this mock kernel
    data_type v = 0;
    if (in_ptr0)
        v += in_ptr0[id];
    if (in_ptr1)
        v += in_ptr1[id];
    if (in_ptr2)
        v += static_cast<data_type>(in_ptr2[0]);
    if (in_ptr3)
        v += static_cast<data_type>(in_ptr3[0]);
    if (in_ptr4)
        v += static_cast<data_type>(in_ptr4[0]);

    if (out_ptr0)
        out_ptr0[id] = v;
    if (out_ptr1)
        out_ptr1[id] = v;
}

template <typename data_type>
void submit_kernel_mock_reshape_and_cache(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                          bool useEvents,
                                          const data_type *in_ptr0,
                                          const data_type *in_ptr1,
                                          data_type *out_ptr0,
                                          data_type *out_ptr1,
                                          long *in_ptr2,
                                          float *in_ptr3,
                                          float *in_ptr4) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_reshape_and_cache<data_type>(item, in_ptr0, in_ptr1, out_ptr0, out_ptr1, in_ptr2, in_ptr3, in_ptr4);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_reshape_and_cache<data_type>(item, in_ptr0, in_ptr1, out_ptr0, out_ptr1, in_ptr2, in_ptr3, in_ptr4);
        });
    }
}

template <typename data_type>
void mock_flash_attn(
    sycl::nd_item<1> item,
    const data_type *in_ptr0,
    const data_type *in_ptr1,
    const data_type *in_ptr2,
    data_type *out_ptr0,
    int *in_ptr3,
    int *in_ptr4,
    int *in_ptr5,
    int *in_ptr6,
    int scalar0,
    int scalar1,
    float scalar2,
    float scalar3,
    bool scalar4,
    bool scalar5,
    int scalar6,
    int scalar7,
    float scalar8,
    bool scalar9) {
    size_t id = item.get_global_id(0);
    id = id % scalar0;
    data_type v = 0;
    if (in_ptr0)
        v += in_ptr0[id];
    if (in_ptr1)
        v += in_ptr1[id];
    if (in_ptr2)
        v += in_ptr2[id];
    if (in_ptr3)
        v += static_cast<data_type>(in_ptr3[0]);
    if (in_ptr4)
        v += static_cast<data_type>(in_ptr4[0]);
    if (in_ptr5)
        v += static_cast<data_type>(in_ptr5[0]);
    if (in_ptr6)
        v += static_cast<data_type>(in_ptr6[0]);

    if (out_ptr0) {
        int use_params = scalar0 + scalar1 + static_cast<int>(scalar2) + static_cast<int>(scalar3) + static_cast<int>(scalar4) +
                         static_cast<int>(scalar5) + scalar6 + scalar7 + static_cast<int>(scalar8) + static_cast<int>(scalar9);
        out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_mock_flash_attn(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                   bool useEvents,
                                   const data_type *in_ptr0,
                                   const data_type *in_ptr1,
                                   const data_type *in_ptr2,
                                   data_type *out_ptr0,
                                   int *in_ptr3,
                                   int *in_ptr4,
                                   int *in_ptr5,
                                   int *in_ptr6,
                                   int scalar0,
                                   int scalar1,
                                   float scalar2,
                                   float scalar3,
                                   bool scalar4,
                                   bool scalar5,
                                   int scalar6,
                                   int scalar7,
                                   float scalar8,
                                   bool scalar9) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_flash_attn<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, in_ptr3, in_ptr4, in_ptr5, in_ptr6, scalar0, scalar1, scalar2, scalar3, scalar4, scalar5, scalar6, scalar7, scalar8, scalar9);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_flash_attn<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, in_ptr3, in_ptr4, in_ptr5, in_ptr6, scalar0, scalar1, scalar2, scalar3, scalar4, scalar5, scalar6, scalar7, scalar8, scalar9);
        });
    }
}

template <typename data_type>
void mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0(
    sycl::nd_item<1> item,
    const data_type *in_ptr0,
    const data_type *in_ptr1,
    const data_type *in_ptr2,
    data_type *out_ptr0,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK) {
    size_t id = item.get_global_id(0);
    id = id % xnumel;
    data_type v = 0;
    if (in_ptr0)
        v += in_ptr0[id];
    if (in_ptr1)
        v += in_ptr1[id];
    if (in_ptr2)
        v += in_ptr2[id];

    if (out_ptr0) {
        int use_params = r0_numel + XBLOCK + R0_BLOCK;
        out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                                                           bool useEvents,
                                                                           const data_type *in_ptr0,
                                                                           const data_type *in_ptr1,
                                                                           const data_type *in_ptr2,
                                                                           data_type *out_ptr0,
                                                                           int xnumel,
                                                                           int r0_numel,
                                                                           int XBLOCK,
                                                                           int R0_BLOCK) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_0<data_type>(item, in_ptr0, in_ptr1, in_ptr2, out_ptr0, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    }
}

template <typename data_type>
void mock_triton_poi_fused_mul_silu_slice_1(
    sycl::nd_item<1> item,
    const data_type *in_ptr0,
    data_type *out_ptr0,
    int xnumel,
    int XBLOCK) {
    size_t id = item.get_global_id(0);
    data_type v = 0;
    if (in_ptr0)
        v += in_ptr0[id];

    if (out_ptr0) {
        int use_params = xnumel + XBLOCK;
        out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_mock_triton_poi_fused_mul_silu_slice_1(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                                          bool useEvents,
                                                          const data_type *in_ptr0,
                                                          data_type *out_ptr0,
                                                          int xnumel,
                                                          int XBLOCK) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_poi_fused_mul_silu_slice_1<data_type>(item, in_ptr0, out_ptr0, xnumel, XBLOCK);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_poi_fused_mul_silu_slice_1<data_type>(item, in_ptr0, out_ptr0, xnumel, XBLOCK);
        });
    }
}

template <typename data_type>
void mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2(
    sycl::nd_item<1> item,
    const data_type *in_ptr0,
    const data_type *in_ptr1,
    const data_type *in_ptr2,
    const data_type *in_ptr3,
    data_type *out_ptr0,
    data_type *out_ptr1,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK) {
    size_t id = item.get_global_id(0);
    id = id % xnumel;
    data_type v = 0;
    if (in_ptr0)
        v += in_ptr0[id];
    if (in_ptr1)
        v += in_ptr1[id];
    if (in_ptr2)
        v += in_ptr2[id];
    if (in_ptr3)
        v += in_ptr3[id];

    if (out_ptr1)
        out_ptr1[id] = v;

    if (out_ptr0) {
        int use_params = r0_numel + XBLOCK + R0_BLOCK;
        out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                                                           bool useEvents,
                                                                           const data_type *in_ptr0,
                                                                           const data_type *in_ptr1,
                                                                           const data_type *in_ptr2,
                                                                           const data_type *in_ptr3,
                                                                           data_type *out_ptr0,
                                                                           data_type *out_ptr1,
                                                                           int xnumel,
                                                                           int r0_numel,
                                                                           int XBLOCK,
                                                                           int R0_BLOCK) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2<data_type>(item, in_ptr0, in_ptr1, in_ptr2, in_ptr3, out_ptr0, out_ptr1, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2<data_type>(item, in_ptr0, in_ptr1, in_ptr2, in_ptr3, out_ptr0, out_ptr1, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    }
}

template <typename data_type>
void mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer(
    sycl::nd_item<1> item,
    data_type *in_out_ptr0,
    const data_type *in_ptr1,
    const data_type *in_ptr2,
    const data_type *in_ptr3,
    int xnumel,
    int r0_numel,
    int XBLOCK,
    int R0_BLOCK) {
    size_t id = item.get_global_id(0);
    id = id % xnumel;
    data_type v = 0;
    if (in_out_ptr0)
        v += in_out_ptr0[id];
    if (in_ptr1)
        v += in_ptr1[id];
    if (in_ptr2)
        v += in_ptr2[id];
    if (in_ptr3)
        v += in_ptr3[id];

    if (in_out_ptr0) {
        int use_params = r0_numel + XBLOCK + R0_BLOCK;
        in_out_ptr0[id] = v + static_cast<data_type>(use_params);
    }
}

template <typename data_type>
void submit_kernel_mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer(unsigned int wgc, unsigned int wgs, sycl::queue &q,
                                                                                      bool useEvents,
                                                                                      data_type *in_out_ptr0,
                                                                                      const data_type *in_ptr1,
                                                                                      const data_type *in_ptr2,
                                                                                      const data_type *in_ptr3,
                                                                                      int xnumel,
                                                                                      int r0_numel,
                                                                                      int XBLOCK,
                                                                                      int R0_BLOCK) {
    if (!useEvents) {
        syclex::nd_launch(q, sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer<data_type>(item, in_out_ptr0, in_ptr1, in_ptr2, in_ptr3, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    } else {
        q.parallel_for(sycl::nd_range<1>{wgc * wgs, wgs}, [=](sycl::nd_item<1> item) {
            mock_triton_red_fused__to_copy_add_mean_mul_pow_rsqrt_2_last_layer<data_type>(item, in_out_ptr0, in_ptr1, in_ptr2, in_ptr3, xnumel, r0_numel, XBLOCK, R0_BLOCK);
        });
    }
}

#endif
