/*
 * Copyright (C) 2022-2025 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include <sycl/sycl.hpp>

namespace SYCL {
namespace sycl = ::sycl;

struct Sycl {
    sycl::device device;
    sycl::queue queue;

    Sycl();
    Sycl(const sycl::device &device);
    Sycl(const sycl::device &device, bool useOOQ);

    template <typename... PropT, typename = typename std::enable_if_t<(sycl::is_property<PropT>::value && ...)>>
    Sycl(PropT... properties) : Sycl(sycl::device{sycl::gpu_selector_v}, properties...) {}

    template <typename... PropT, typename = typename std::enable_if_t<(sycl::is_property<PropT>::value && ...)>>
    Sycl(const sycl::device &device, PropT... properties) : device{device} {
        if (Configuration::get().useOOQ) {
            queue = sycl::queue(device, sycl::property_list{properties...});
        } else {
            queue = sycl::queue(device, sycl::property_list{sycl::property::queue::in_order(), properties...});
        }
    }

    ~Sycl();
};
} // namespace SYCL

using namespace SYCL;
