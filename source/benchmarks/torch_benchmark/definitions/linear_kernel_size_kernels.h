/*
 * Copyright (C) 2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <sycl/sycl.hpp>

namespace syclext = sycl::ext::oneapi;
namespace syclexp = sycl::ext::oneapi::experimental;

#define INIT_DATA_LINE(i) data[i] = static_cast<data_type>(1);
#define INIT_DATA_2(i) INIT_DATA_LINE(i) INIT_DATA_LINE(1 + i)
#define INIT_DATA_4(i) INIT_DATA_2(i) INIT_DATA_2(2 + i)
#define INIT_DATA_8(i) INIT_DATA_4(i) INIT_DATA_4(4 + i)
#define INIT_DATA_16(i) INIT_DATA_8(i) INIT_DATA_8(8 + i)
#define INIT_DATA_32(i) INIT_DATA_16(i) INIT_DATA_16(16 + i)
#define INIT_DATA_64(i) INIT_DATA_32(i) INIT_DATA_32(32 + i)
#define INIT_DATA_128(i) INIT_DATA_64(i) INIT_DATA_64(64 + i)
#define INIT_DATA_256(i) INIT_DATA_128(i) INIT_DATA_128(128 + i)
#define INIT_DATA_512(i) INIT_DATA_256(i) INIT_DATA_256(256 + i)
#define INIT_DATA_1024(i) INIT_DATA_512(i) INIT_DATA_512(512 + i)
#define INIT_DATA_2048(i) INIT_DATA_1024(i) INIT_DATA_1024(1024 + i)
#define INIT_DATA_4096(i) INIT_DATA_2048(i) INIT_DATA_2048(2048 + i)

// This file contains multiple kernel definitions with different array sizes.

template <typename data_type>
SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((syclexp::nd_range_kernel<1>))
void linear_kernel_size_32(data_type *out) {
    constexpr size_t ARRAY_SIZE = 32;
    data_type data[ARRAY_SIZE];

    INIT_DATA_32(0)

    data_type sum = static_cast<data_type>(0);
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        sum += data[i];
    }
    out[0] = sum;
}

template <typename data_type>
SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((syclexp::nd_range_kernel<1>))
void linear_kernel_size_128(data_type *out) {
    constexpr size_t ARRAY_SIZE = 128;
    data_type data[ARRAY_SIZE];

    INIT_DATA_128(0)

    data_type sum = static_cast<data_type>(0);
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        sum += data[i];
    }
    out[0] = sum;
}

template <typename data_type>
SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((syclexp::nd_range_kernel<1>))
void linear_kernel_size_512(data_type *out) {
    constexpr size_t ARRAY_SIZE = 512;
    data_type data[ARRAY_SIZE];

    INIT_DATA_512(0)

    data_type sum = static_cast<data_type>(0);
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        sum += data[i];
    }
    out[0] = sum;
}

template <typename data_type>
SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((syclexp::nd_range_kernel<1>))
void linear_kernel_size_1024(data_type *out) {
    constexpr size_t ARRAY_SIZE = 1024;
    data_type data[ARRAY_SIZE];

    INIT_DATA_1024(0)

    data_type sum = static_cast<data_type>(0);
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        sum += data[i];
    }
    out[0] = sum;
}

template <typename data_type>
SYCL_EXT_ONEAPI_FUNCTION_PROPERTY((syclexp::nd_range_kernel<1>))
void linear_kernel_size_5120(data_type *out) {
    constexpr size_t ARRAY_SIZE = 5120;
    data_type data[ARRAY_SIZE];

    INIT_DATA_4096(0)
    INIT_DATA_1024(4096)

    data_type sum = static_cast<data_type>(0);
    for (size_t i = 0; i < ARRAY_SIZE; i++) {
        sum += data[i];
    }
    out[0] = sum;
}
