/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "sycl.h"

namespace SYCL {
Sycl::Sycl() : Sycl(sycl::device{sycl::gpu_selector_v}) {}

Sycl::Sycl(const sycl::device &device) : Sycl(device, Configuration::get().useOOQ) {}

Sycl::Sycl(const sycl::device &device, bool useOOQ) : device{device} {
    if (useOOQ) {
        queue = sycl::queue(device);
    } else {
        queue = sycl::queue(device, sycl::property_list{sycl::property::queue::in_order()});
    }
}

Sycl::~Sycl() {}
} // namespace SYCL
