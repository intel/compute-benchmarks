/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "framework/test_case/test_case.h"

#include <CL/sycl.hpp>

namespace SYCL {
namespace sycl = ::sycl;

struct Sycl {
    sycl::device device;
    sycl::queue queue;

    Sycl();
    Sycl(const sycl::device_selector &deviceSelector);

    template <typename... PropT, typename = typename std::enable_if_t<(sycl::is_property<PropT>::value && ...)>>
    Sycl(PropT... properties) : Sycl(sycl::gpu_selector{}, properties...) {}

    template <typename... PropT, typename = typename std::enable_if_t<(sycl::is_property<PropT>::value && ...)>>
    Sycl(const sycl::device_selector &deviceSelector, PropT... properties) : device(deviceSelector) {
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
