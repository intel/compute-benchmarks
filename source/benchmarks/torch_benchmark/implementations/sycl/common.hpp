/*
 * Copyright (C) 2026 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/sycl/sycl.h"
#include "framework/test_case/register_test_case.h"
#include "framework/utility/combo_profiler.h"

#include "definitions/sycl_kernels.h"

constexpr float epsilon = 1e-5f;

namespace detail {
template <typename data_type>
auto make_sycl_deleter(sycl::queue queue) {
    return [queue](data_type *ptr) {
        if (ptr)
            sycl::free(ptr, queue);
    };
}
} // namespace detail

template <typename data_type>
auto make_device_ptr(Sycl &sycl, size_t size) {
    auto deleter = detail::make_sycl_deleter<data_type>(sycl.queue);
    using unique_ptr_type = std::unique_ptr<data_type, decltype(deleter)>;
    auto ptr = sycl::malloc_device<data_type>(size, sycl.queue);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return unique_ptr_type(ptr, deleter);
}

template <typename data_type>
auto make_host_ptr(Sycl &sycl, size_t size) {
    auto deleter = detail::make_sycl_deleter<data_type>(sycl.queue);
    using unique_ptr_type = std::unique_ptr<data_type, decltype(deleter)>;
    auto ptr = sycl::malloc_host<data_type>(size, sycl.queue);
    if (!ptr) {
        throw std::bad_alloc();
    }
    return unique_ptr_type(ptr, deleter);
}
