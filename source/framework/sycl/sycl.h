/*
 * Copyright (C) 2022 Intel Corporation
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
    Sycl(const sycl::property_list &propertyList);
    Sycl(const sycl::device_selector &deviceSelector, const sycl::property_list &propertyList);
    ~Sycl();
};
} // namespace SYCL

using namespace SYCL;
